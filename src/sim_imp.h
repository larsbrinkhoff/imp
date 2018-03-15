/* sim_imp.h: 1822 IMP interface.
  ------------------------------------------------------------------------------

   Copyright (c) 2018, Lars Brinkhoff

  ------------------------------------------------------------------------------
*/

#ifndef SIM_IMP_H
#define SIM_IMP_H

#include "sim_defs.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef unsigned long long int uint64;

struct imp_device {
  t_stat (*receive_bit) (int, int);
/*t_stat (*receive_packet) (uint64, int);
  t_stat (*imp_ready) (void);*/

  int imp_error;
  int host_error;

  uint8 packet_to_imp[1020];
  uint8 packet_to_host[1020];
  size_t bits_to_imp;
  size_t bits_to_host;
};

typedef struct imp_device IMP;

t_stat imp_reset (IMP *);

/* Send bits from host to IMP. */
t_stat imp_send_bits (IMP *, uint64 bits, int n, int last);

/* Send a complete packet from host to IMP. */
t_stat imp_send_packet (IMP *, void *octets, int n);

/* Send a complete packet from IMP to host. */
t_stat imp_receive_packet (IMP *, void *octets, int n);

/* Signal host ready. */
t_stat imp_host_ready (IMP *, int);

#ifdef  __cplusplus
}
#endif

#endif /* SIM_IMP_H */
