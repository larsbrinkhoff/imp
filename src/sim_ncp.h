/* sim_ncp.h: Emulate the IMP Network Control Program.
  ------------------------------------------------------------------------------

   Copyright (c) 2018, Lars Brinkhoff

  ------------------------------------------------------------------------------
*/

#ifndef SIM_NCP_H
#define SIM_NCP_H

#include "sim_defs.h"
#include "sim_imp.h"

#ifdef  __cplusplus
extern "C" {
#endif

t_stat ncp_reset (IMP *);
t_stat ncp_process (uint8 *packet);

#ifdef  __cplusplus
}
#endif

#endif /* SIM_NCP_H */
