#ifndef HEADER_H_

#define HEADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <signal.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "cache.h"
#include "loader.h"
#include "syscall.h"
#include "bpred.h"
#include "resource.h"
#include "bitmap.h"
#include "options.h"
#include "eval.h"
#include "stats.h"
#include "ptrace.h"
#include "dlite.h"
#include "sim.h"
#include "network.h"
#include "mystats.h"

//#include "point-point.h"
#include "opticalForSimplescalar/point-point.h"

/*  for defining SMT   */
#include "smt.h"

#include "interconnect.h"
//#include "power_dynamic.h"

#include "context.h"

/* added for Wattch */
//#include "power.h"

#include "cluster.h"

#include "cluster-opts.h"

#include "hotspot.h"

#include "cmt.h"

/* Garg */
#include "MTA.h"

#include "tokencoherence.h"


/* stuff found in sim-outorder */




#define PCB_LINE_SIZE 16
extern counter_t sim_cycle;
extern int actual_clusters;
extern double tot_del;
extern double tot_del_bw;
extern double num_bw_of;
extern int RUU_size;
#ifdef  COMM_BUFF
extern counter_t comm1qram_access_cl[];
extern counter_t comm2qram_access_cl[];
#endif
extern int hotspot_cycle_sampling_intvl;

#endif
