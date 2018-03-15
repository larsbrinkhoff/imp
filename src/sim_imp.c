/* sim_imp.c: Generic IMP 1822 interface.
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

  Transmit data over a 1822 interface.

  ------------------------------------------------------------------------------
*/

#include "sim_imp.h"
#include "sim_ncp.h"

static t_stat imp_svc (UNIT *);

UNIT imp_unit[1] = {{ UDATA (imp_svc, 0, 0)}};

static DEVICE imp_dev = {
    "IMP", imp_unit, NULL, NULL, 
    0, 0, 0, 0, 0, 0, 
    NULL, NULL, NULL, NULL, NULL, NULL, 
    NULL, DEV_DEBUG | DEV_NOSAVE, 0, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL};

static int ready;

t_stat imp_reset (IMP *imp)
{
  memset (imp, 0, sizeof (IMP));
  ready = 1;
  sim_register_internal_device (&imp_dev);
  imp_unit[0].up7 = imp;
  ncp_reset (imp);
  return SCPE_OK;
}

/* Send bits from host to IMP. */
t_stat imp_send_bits (IMP *imp, uint64 bits, int n, int last)
{
  int i;
  int bit;

  bit = imp->bits_to_imp;
  for (i = n-1; i >= 0; i--) {
    imp->packet_to_imp[bit >> 3] |= ((bits >> i) & 1) << (7-(bit & 7));
    bit++;
  }
  imp->bits_to_imp = bit;

  if (last) {
    imp_send_packet (imp, imp->packet_to_imp, imp->bits_to_imp);
    imp->bits_to_imp = 0;
    memset (imp->packet_to_imp, 0, sizeof imp->packet_to_imp);
  }

  return SCPE_OK;
}

static void print_packet (FILE *f, uint8 *octets, int n)
{
  int i;
  for (i = 0; i < n; i += 4)
    fprintf (stderr, "%X", (octets[i >> 3] >> (4-(i&4))) & 017);
}

/* Send a complete packet to IMP. */
t_stat imp_send_packet (IMP *imp, void *packet, int n)
{
  uint8 *octets = packet;
  //fprintf (stderr, "IMP from host: ");
  //print_packet (stderr, octets, n);
  //fprintf (stderr, "\r\n");
  ncp_process (octets);
}

int bit;
int bits = 0;

/* Send a complete packet to the host. */
t_stat imp_receive_packet (IMP *imp, void *packet, int n)
{
  int i;

  if (!ready)
    return SCPE_UDIS;

  //fprintf (stderr, "IMP to host: ");
  //print_packet (stderr, octets, n);
  //fprintf (stderr, "\r\n");

  memcpy (imp->packet_to_host, packet, n/8);
  ready = 0;
  bits = n;
  bit = 0;

  sim_activate (imp_unit, 2000);

  return SCPE_OK;
}

static t_stat imp_svc (UNIT *uptr)
{
  IMP *imp = uptr->up7;
  if (bits > 0) {
    if (imp->receive_bit ((imp->packet_to_host[bit >> 3] >> (7-(bit&7))) & 1,
                          bits == 1) == SCPE_OK) {
      bit++;
      bits--;
      ready = (bits == 0);
    }
  }
  sim_activate (imp_unit, 2000);
  return SCPE_OK;
}
