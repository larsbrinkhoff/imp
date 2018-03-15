#include "sim_defs.h"
#include "sim_imp.h"

IMP imp;

t_stat sim_register_internal_device (DEVICE *dptr)
{
  return SCPE_OK;
}

t_stat sim_activate (UNIT *uptr, int interval)
{
  return SCPE_OK;
}

t_stat receive_bit (int bit, int last)
{
  return SCPE_OK;
}

int main (void)
{
  imp_reset (&imp);
  imp.receive_bit = receive_bit;

  /* Main loop. */

  for (;;) {
    /* Wait for data. */

    /* Send to imp_send_bits or imp_send_packet. */
  }

  return 0;
}
