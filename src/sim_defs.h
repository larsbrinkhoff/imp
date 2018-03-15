#ifndef SIM_DEFS
#define SIM_DEFS

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

typedef int t_stat;
typedef unsigned char uint8;
typedef unsigned char uint32;

typedef struct {
  void *up7;
} UNIT;
#define UDATA(x,y,z)

#define DEV_DEBUG 1
#define DEV_NOSAVE 2

#define SCPE_OK 0
#define SCPE_UDIS 1
#define SCPE_ARG 2
#define SCPE_UNATT 3

typedef struct {
  char *x1;
  UNIT *u;
  void *x2, *x3;
  int x4, x5, x6, x7, x8, x9a;
  void *x9, *x9b, *x9c, *x9d, *x9e, *x9f;
  void *x9g;
  int x10, x11;
  void *x12, *x13, *x14, *x15, *x16, *x17, *x18;
} DEVICE;

extern t_stat sim_register_internal_device (DEVICE *);
extern t_stat sim_activate (UNIT *, int);

#endif /* SIM_DEFS */
