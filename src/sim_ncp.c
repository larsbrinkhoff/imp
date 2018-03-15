/* sim_ncp.c: Emulate the IMP Network Control Program.

  ------------------------------------------------------------------------------
   Copyright (c) 2018, Lars Brinkhoff

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

   Except as contained in this notice, the name of the author shall not be
   used in advertising or otherwise to promote the sale, use or other dealings
   in this Software without prior written authorization from the author.

  ------------------------------------------------------------------------------
*/

#include <poll.h>
#include <unistd.h>

#include "sim_ncp.h"
#include "sim_imp.h"
#include "sim_tun.h"

static t_stat ncp_svc (UNIT *);

UNIT ncp_unit[1] = {{ UDATA (ncp_svc, 0, 0)}};

static DEVICE ncp = {
    "NCP", ncp_unit, NULL, NULL, 
    0, 0, 0, 0, 0, 0, 
    NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, DEV_DEBUG | DEV_NOSAVE, 0, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL};

static IMP *imp;
static int init_state = 0;
static int padding = 0;
static int tun = -1;

static t_stat process_ip (uint8 *packet)
{
  int n1, n2;

  n1 = (packet[2] << 8) + packet[3];
  n2 = write (tun, packet, n1);
  if (n2 == -1)
    fprintf (stderr, "TUN write error %d: %s\r\n", errno, strerror (errno));
}

static t_stat process_regular (uint8 *packet)
{
  int n = (packet[10] << 8) + packet[11];
  //fprintf (stderr, "NCP: Length %d\r\n", n);

  switch (packet[9] & 0xF)
    {
    case 0:
      //fprintf (stderr, "NCP: Standard, non refusable\r\n");
      if (packet[8] == 0233)
        process_ip (packet + 12 + padding/8);
      else if (packet[8] == 0)
        ; // Host-to-host control messages.
      else if (packet[8] >= 2 && packet[8] <= 0107)
        ; // Host-to-host regular messages.
      else
        ; // Unknown
      break;
    case 1:
      fprintf (stderr, "NCP: Refusable\r\n");
      break;
    case 2:
      fprintf (stderr, "NCP: Get ready\r\n");
      break;
    case 3:
      fprintf (stderr, "NCP: Uncontrolled\r\n");
      break;
    default:
      fprintf (stderr, "NCP: Unassigned\r\n");
      break;
    }

    return SCPE_OK;
}

int local_host_id = 0206; // MIT-AI ARPAnet host number
int local_imp_id = 070707; // Random number
uint8 packet[1000];

static void send_nop (void)
{
  memset (packet, 0, sizeof packet);
  packet[0] = 0x0F; // new format
  packet[3] = 0x04; // NOP
  packet[5] = local_host_id;
  packet[6] = local_imp_id >> 8;
  packet[7] = local_imp_id & 0xFF;

  if (imp_receive_packet (imp, &packet, 96) == SCPE_OK)
    init_state++;
}

static void send_rfnm (void)
{
  memset (packet, 0, sizeof packet);
  packet[0] = 0x0F; // new format
  packet[3] = 5; // Ready for next message

  if (imp_receive_packet (imp, &packet, 96) == SCPE_OK)
    init_state++;
}

static void send_reset (void)
{
  memset (packet, 0, sizeof packet);
  packet[0] = 0x0F; // new format
  packet[3] = 10; // Interface reset

  if (imp_receive_packet (imp, &packet, 96) == SCPE_OK)
    init_state++;
}

static int ip_checksum (uint8 *packet, int n)
{
  int i;
  uint32 checksum = 0;

  for (i = 0; i < n; i += 2) {
    checksum += packet[i] << 8;
    checksum += packet[i+1];
    while (checksum > 0xFFFF)
      checksum = (checksum & 0xFFFF) + 1;
  }

  return ~checksum;
}

static t_stat send_ip (void *payload, int n)
{
  int checksum;

  memset (packet, 0, sizeof packet);

  // 1822 leader.
  packet[0] = 0x0F; // new format
  packet[3] = 0; // Host-to-host
  packet[8] = 0233; // IP

  // IP header.
  packet[12+padding/8] = (4<<4) + 5; // IP version, header 32-bit words
  packet[13+padding/8] = 0; // Type of service
  packet[14+padding/8] = 0; // Length
  packet[15+padding/8] = 28; // Length
  packet[16+padding/8] = 0; // ID
  packet[17+padding/8] = 0; // ID
  packet[18+padding/8] = 0; // Flags, fragment offset
  packet[19+padding/8] = 0; // Fragment offset
  packet[20+padding/8] = 9; // Time to live
  packet[21+padding/8] = 1; // Protocol = ICMP
  packet[24+padding/8] = 10; // Source address
  packet[25+padding/8] = 3;
  packet[26+padding/8] = 0;
  packet[27+padding/8] = 99;
  packet[28+padding/8] = 10; // Destination address
  packet[29+padding/8] = 3;
  packet[30+padding/8] = 0;
  packet[31+padding/8] = 6;

  checksum = ip_checksum (packet+12+padding/8, 20);
  packet[22+padding/8] = checksum >> 8;
  packet[23+padding/8] = checksum & 0xFF;

  memcpy (packet + 12 + padding/8 + 20, payload, n);

  return imp_receive_packet (imp, &packet, 96 + padding + 5*32 + 8*n);
}

t_stat ncp_process (uint8 *packet)
{
  if ((packet[0] & 0x0F) == 13) {
    fprintf (stderr, "NCP: 1822L packet format.\r\n");
    return SCPE_ARG;
  }

  if ((packet[0] & 0x0F) != 0x0F) {
    fprintf (stderr, "NCP: old-style packet format.\r\n");
    return SCPE_ARG;
  }

  switch (packet[3])
    {
    case 0:
      //fprintf (stderr, "NCP: Regular.\r\n");
      return process_regular (packet);
    case 1:
      fprintf (stderr, "NCP: Error.\r\n");
      break;
    case 2:
      fprintf (stderr, "NCP: Host going down.\r\n");
      break;
    case 4:
      padding = packet[9] & 0xF;
      if (init_state < 3)
        init_state++;
      //fprintf (stderr, "NCP: Nop.  padding=%d\r\n", padding);
      padding *= 16;
      break;
    case 8:
      fprintf (stderr, "NCP: Error.\r\n");
      break;
    default:
      fprintf (stderr, "NCP: Unassigned.\r\n");
      break;
    }

    return SCPE_OK;
}

t_stat ncp_reset (IMP *i)
{
  char ifname[100];
  //fprintf (stderr, "NCP: reset\r\n");

  imp = i;
  init_state = 0;
  sim_register_internal_device (&ncp);
  sim_activate (ncp_unit, 10000);

  if (tun == -1) {
    strcpy (ifname, "imp0");
    tun = tun_alloc (ifname);
    fprintf (stderr, "NCP: tun %s fd = %d\r\n", ifname, tun);
    if (tun < 0)
      return SCPE_UNATT;

    system ("ifconfig imp0 up");
    system ("route add 10.3.0.6 imp0");
  }

  return SCPE_OK;
}

int octets = 0;
uint8 buffer[1500];

static void process_buffer (void)
{
  uint8 message[1000];

  if (octets == 0)
    return;

  memset (message, 0, sizeof message);

  // 1822 leader.
  message[0] = 0x0F; // new format
  message[3] = 0; // Host-to-host
  message[8] = 0233; // IP

  memcpy (message + 12 + padding/8, buffer, octets);

  if (imp_receive_packet (imp, message, 96 + padding + 8*octets) == SCPE_OK)
    octets = 0;
}

static void check_ether (void)
{
  struct pollfd fd[1];
  int n1, n2, n3;

  if (octets > 0)
    return;

  fd[0].fd = tun;
  fd[0].events = POLLIN;

  if (poll (fd, 1, 0) > 0) {
    n1 = read (tun, buffer, sizeof buffer);
    if (n1 == -1)
      fprintf (stderr, "TUN read error %d: %s\r\n", errno, strerror (errno));
    else
      octets = n1;
  }  
}

static t_stat ncp_svc (UNIT *uptr)
{
  if (init_state >= 3 && init_state < 6)
      send_nop ();
  else if (init_state == 6)
      send_reset ();
  else if (init_state > 6) {
      check_ether ();
      process_buffer ();
  }
  
  sim_activate (ncp_unit, 10000);
  return SCPE_OK;
}
