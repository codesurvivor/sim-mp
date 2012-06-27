#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/uio.h>
#include <errno.h>

#include <malloc.h>
#include <sys/mman.h>
#define  SMT_SS
#include "smt.h" 

#ifdef SMT_SS
#include "context.h"
#endif


#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "sim.h"
#include "endian.h"
#include "eio.h"
#include "syscall.h"
#include "target-risc/syscall.h"
#include "target-risc/mmap.h"

#if defined(MTA)
#include "MTA.h"
#endif
extern md_addr_t ld_brk_point;
int barrier_waiting[MAXTHREADS];


/* live execution only support on same-endian hosts... */
#ifndef MD_CROSS_ENDIAN

#ifdef _MSC_VER
#include <io.h>
#else /* !_MSC_VER */
#include <unistd.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#ifndef _MSC_VER
#include <sys/param.h>
#endif
#include <errno.h>
#include <time.h>
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#ifndef _MSC_VER
#include <sys/resource.h>
#endif
#include <signal.h>

/* #include <sys/file.h> */

#include <sys/stat.h>
#ifndef _MSC_VER
#include <sys/uio.h>
#endif
#include <setjmp.h>
#ifndef _MSC_VER
#include <sys/times.h>
#endif
#include <limits.h>
#ifndef _MSC_VER
#include <sys/ioctl.h>
#endif
#if !defined(linux) && !defined(sparc) && !defined(hpux) && !defined(__hpux) && !defined(__CYGWIN32__) && !defined(ultrix)
#ifndef _MSC_VER
#include <sys/select.h>
#endif
#endif
#ifdef linux
#include <utime.h>
#include <sgtty.h>
#endif /* linux */

#if defined(hpux) || defined(__hpux)
#include <sgtty.h>
#endif

#ifdef __svr4__
#include "utime.h"
#endif

#if defined(sparc) && defined(__unix__)
#if defined(__svr4__) || defined(__USLC__)
#include <dirent.h>
#else
#include <sys/dir.h>
#endif

/* dorks */
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef TAB2
#undef XTABS
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef ECHO
#undef NOFLSH
#undef TOSTOP
#undef FLUSHO
#undef PENDIN
#endif

#if defined(hpux) || defined(__hpux)
#undef CR0
#endif

#ifdef __FreeBSD__
#include <termios.h>
/*#include <sys/ioctl_compat.h>*/
#else /* !__FreeBSD__ */
#ifndef _MSC_VER
#include <termio.h>
#endif
#endif

#if defined(hpux) || defined(__hpux)
/* et tu, dorks! */
#undef HUPCL
#undef ECHO
#undef B50
#undef B75
#undef B110
#undef B134
#undef B150
#undef B200
#undef B300
#undef B600
#undef B1200
#undef B1800
#undef B2400
#undef B4800
#undef B9600
#undef B19200
#undef B38400
#undef NL0
#undef NL1
#undef CR0
#undef CR1
#undef CR2
#undef CR3
#undef TAB0
#undef TAB1
#undef BS0
#undef BS1
#undef FF0
#undef FF1
#undef EXTA
#undef EXTB
#undef B900
#undef B3600
#undef B7200
#undef XTABS
#include <sgtty.h>
#include <utime.h>
#endif
static md_addr_t mmap_brk_point = 0x40000000;


/* target stat() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct ss_statbuf
{
  shalf_t ss_st_dev;
  shalf_t ss_pad;
  word_t ss_st_ino;
  half_t ss_st_mode;
  shalf_t ss_st_nlink;
  shalf_t ss_st_uid;
  shalf_t ss_st_gid;
  shalf_t ss_st_rdev;
  shalf_t ss_pad1;
  sword_t ss_st_size;
  sword_t ss_st_atime;
  sword_t ss_st_spare1;
  sword_t ss_st_mtime;
  sword_t ss_st_spare2;
  sword_t ss_st_ctime;
  sword_t ss_st_spare3;
  sword_t ss_st_blksize;
  sword_t ss_st_blocks;
  word_t ss_st_gennum;
  sword_t ss_st_spare4;
};

struct ss_sgttyb {
  byte_t sg_ispeed;     /* input speed */
  byte_t sg_ospeed;     /* output speed */
  byte_t sg_erase;      /* erase character */
  byte_t sg_kill;       /* kill character */
  shalf_t sg_flags;     /* mode flags */
};

struct linux_tms
{
  word_t tms_utime;		/* user CPU time */
  word_t tms_stime;		/* system CPU time */

  word_t tms_cutime;		/* user CPU time of dead children */
  word_t tms_cstime;		/* system CPU time of dead children */

};
struct ss_timeval
{
  sword_t ss_tv_sec;		/* seconds */
  sword_t ss_tv_usec;		/* microseconds */
};

/* target getrusage() buffer definition, the host stat buffer format is
   automagically mapped to/from this format in syscall.c */
struct ss_rusage
{
  struct ss_timeval ss_ru_utime;
  struct ss_timeval ss_ru_stime;
  sword_t ss_ru_maxrss;
  sword_t ss_ru_ixrss;
  sword_t ss_ru_idrss;
  sword_t ss_ru_isrss;
  sword_t ss_ru_minflt;
  sword_t ss_ru_majflt;
  sword_t ss_ru_nswap;
  sword_t ss_ru_inblock;
  sword_t ss_ru_oublock;
  sword_t ss_ru_msgsnd;
  sword_t ss_ru_msgrcv;
  sword_t ss_ru_nsignals;
  sword_t ss_ru_nvcsw;
  sword_t ss_ru_nivcsw;
};

struct ss_timezone
{
  sword_t ss_tz_minuteswest;	/* minutes west of Greenwich */
  sword_t ss_tz_dsttime;	/* type of dst correction */
};

struct ss_rlimit
{
  int ss_rlim_cur;		/* current (soft) limit */
  int ss_rlim_max;		/* maximum value for rlim_cur */
};
#define CV_BMAP_SZ              (BITMAP_SIZE(MD_TOTAL_REGS))

int lock_waiting[MAXTHREADS];
extern int priority[MAXTHREADS];
extern int key[MAXTHREADS];


extern FILE *sim_progfd;

extern counter_t TotalBarriers;
extern counter_t TotalLocks;
void general_stat (int id);
void MTA_printInfo (int id);
int tmpNum;
extern int numThreads[CLUSTERS];
extern int mta_maxthreads;
extern int collect_stats;
extern int flushImpStats;

extern int collect_barrier_stats[CLUSTERS];
extern int collect_barrier_release;
md_addr_t barrier_addr;
int collect_barrier_stats_own[CLUSTERS];
extern int collect_lock_stats[CLUSTERS];
extern md_addr_t LockInitPC;
extern counter_t sim_cycle;
extern int startTakingStatistics;
extern int currentPhase;
extern counter_t parallel_sim_cycle;
extern struct quiesceStruct quiesceAddrStruct[CLUSTERS];
void flushWriteBuffer (int threadid);
extern int fastfwd;
extern int stopsim;
extern int access_mem;
extern int access_mem_id;
extern struct cache_t *cache_dl1[CLUSTERS];
extern int collectStatBarrier, collectStatStop[MAXTHREADS], timeToReturn;
extern FILE *outFile;
extern counter_t sleepCount[];
extern long long int freezeCounter;
extern int threadForked[4];
extern struct CV_link CVLINK_NULL;
extern int RUU_size;

/* open(2) flags for SimpleScalar target, syscall.c automagically maps
   between these codes to/from host open(2) flags */
#define SS_O_RDONLY		0
#define SS_O_WRONLY		1
#define SS_O_RDWR		2
#define SS_O_CREAT		0x0100
//#define SS_O_EXCL		0x0400
#define SS_O_EXCL		0x0200
//#define SS_O_NOCTTY		0x0800
#define SS_O_NOCTTY		0x0400
#define SS_O_TRUNC		0x1000
//#define SS_O_APPEND		0x0008
#define SS_O_APPEND		0x02000
//#define SS_O_NONBLOCK		0x0080
#define SS_O_NONBLOCK		0x4000
//#define SS_O_SYNC		0x0010
#define SS_O_SYNC		0x10000
#define SS_O_ASYNC		0x020000
/* open(2) flags translation table for SimpleScalar target */
struct {
  int ss_flag;
  int local_flag;
} ss_flag_table[] = {
  /* target flag */	/* host flag */
#ifdef _MSC_VER
  { SS_O_RDONLY,	_O_RDONLY },
  { SS_O_WRONLY,	_O_WRONLY },
  { SS_O_RDWR,		_O_RDWR },
  { SS_O_APPEND,	_O_APPEND },
  { SS_O_CREAT,		_O_CREAT },
  { SS_O_TRUNC,		_O_TRUNC },
  { SS_O_EXCL,		_O_EXCL },
#ifdef _O_NONBLOCK
  { SS_O_NONBLOCK,	_O_NONBLOCK },
#endif
#ifdef _O_NOCTTY
  { SS_O_NOCTTY,	_O_NOCTTY },
#endif
#ifdef _O_SYNC
  { SS_O_SYNC,		_O_SYNC },
#endif
#else /* !_MSC_VER */
  { SS_O_RDONLY,	O_RDONLY },
  { SS_O_WRONLY,	O_WRONLY },
  { SS_O_RDWR,		O_RDWR },
  { SS_O_APPEND,	O_APPEND },
  { SS_O_CREAT,		O_CREAT },
  { SS_O_TRUNC,		O_TRUNC },
  { SS_O_EXCL,		O_EXCL },
  { SS_O_NONBLOCK,	O_NONBLOCK },
  { SS_O_NOCTTY,	O_NOCTTY },
#ifdef O_SYNC
  { SS_O_SYNC,		O_SYNC },
#endif
#endif /* _MSC_VER */
};
#define SS_NFLAGS	(sizeof(ss_flag_table)/sizeof(ss_flag_table[0]))

#endif /* !MD_CROSS_ENDIAN */
void print_stats (context * current)
{
    printf ("Total number of instructions executed = %d (%d)\n", current->sim_num_insn, current->id);

    return;
}
void
AddToTheSharedAddrList (unsigned long long addr, unsigned int size)
{
    struct sharedAddressList_s *tmpPtr1, *tmpPtr2;
    int indx;
    indx = (int) (((int) addr & 1016) >> 3);
    indx = 0;

    if (sharedAddressList[indx] == NULL)
    {
	sharedAddressList[indx] = (struct sharedAddressList_s *) malloc (sizeof (struct sharedAddressList_s));
	sharedAddressList[indx]->address = addr;
	sharedAddressList[indx]->size = size;
	sharedAddressList[indx]->next = NULL;
    }
    else
    {
   	tmpPtr1 = sharedAddressList[indx];
	tmpPtr2 = sharedAddressList[indx];
	while (tmpPtr1 != NULL)
	{
	    tmpPtr2 = tmpPtr1;
   	    tmpPtr1 = tmpPtr1->next;
    	}
	tmpPtr2->next = (struct sharedAddressList_s *) malloc (sizeof (struct sharedAddressList_s));
	tmpPtr2->next->address = addr;
	tmpPtr2->next->size = size;
	tmpPtr2->next->next = NULL;
   }
}
/* syscall proxy handler, architect registers and memory are assumed to be
   precise when this function is called, register and memory are updated with
   the results of the sustem call */
void
sys_syscall(struct regs_t *regs,	/* registers to access */
	    mem_access_fn mem_fn,	/* generic memory accessor */
	    struct mem_t *mem,		/* memory space to access */
	    md_inst_t inst,		/* system call inst */
	    int traceable)		/* traceable system call? */
{
  word_t syscode = regs->regs_R[2];
	#ifdef SMT_SS
#if defined(MTA)
    char *p;
#endif
    int cnt;
    int threadid = 0;
    struct mem_pte_t *tmpMemPtrD, *tmpMemPtrS, *tmpMemPtr;
    context *current;
    context *newContext;

    threadid = regs->threadid;
    current = thecontexts[threadid];
#endif
  /* first, check if an EIO trace is being consumed... */
  if (traceable && sim_eio_fd != NULL)
    {
      eio_read_trace(sim_eio_fd, sim_num_insn, regs, mem_fn, mem, inst);

      /* fini... */
      return;
    }
#ifdef MD_CROSS_ENDIAN
  else if (syscode == SS_SYS_exit)
    {
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf, /* exitcode + fudge */regs->regs_R[4]+1);
    }
  else
    fatal("cannot execute PISA system call on cross-endian host");

#else /* !MD_CROSS_ENDIAN */

  /* no, OK execute the live system call... */
  switch (syscode)
    {

	/* syscalls used for sync*/
	/*by wxh*/
//--------------------------------------------
    case OSF_SYS_printf:
//      fprintf(stderr, "printf %d...%d\n", current->id, regs->regs_R[MD_REG_A0]);      
	fflush (stderr);
	regs->regs_R[MD_REG_V0] = 0;
	regs->regs_R[MD_REG_A3] = 0;
	break;
//--------------------------------------------
    case OSF_SYS_barrier:
	if (current->parent == -1)
	{
	    tmpNum = numThreads[current->id] = numThreads[current->id] - 1;
	}
	else
	{
	    tmpNum = numThreads[current->parent] = numThreads[current->parent] - 1;
	}

	if (tmpNum == 0)
	{
	    fprintf (stderr, "BARRIER\n");
	    for (cnt = 0; cnt < numcontexts; cnt++)
	    {
		thecontexts[cnt]->running = 1;
	    }
	    if (current->parent == -1)
	    {
		numThreads[current->id] = mta_maxthreads;
	    }
	    else
	    {
		numThreads[current->parent] = mta_maxthreads;
	    }
	    if (startTakingStatistics == 1)
		currentPhase = 7;
	    startTakingStatistics = 1;
	}
	else
	{
	    current->running = 0;
	}
	regs->regs_R[MD_REG_V0] = 0;
	regs->regs_R[MD_REG_A3] = 0;

	break;
//--------------------------------------------
    case OSF_SYS_SHDADDR:
	AddToTheSharedAddrList ((unsigned long long) regs->regs_R[MD_REG_A0], (unsigned int) regs->regs_R[MD_REG_A1]);
	regs->regs_R[MD_REG_V0] = 0;
	regs->regs_R[MD_REG_A3] = 0;
	break;
//--------------------------------------------
    case OSF_SYS_WAIT:
	/* If  the thread is a child,
	   a) Print the stats
	   b) Stop it.
	   c) Reduce the number of contexts.
	   d) Update the steering list
	   e) return 0;
	   If it is a parent
	   a) If the number of threads is 0, return 1
	   else return -1
	 */
	if (current->parent != -1)
	{
	    fprintf (stderr, "***************Thread %d is halting**************\n", current->id);
	    fprintf (stderr, "TotBarCycle = %8ld, TotBarInstr = %8ld\n", (unsigned long) current->totalBarrierCycle, (unsigned long) current->totalBarrierInstr);
	    fprintf (stderr, "TotLockCycle = %8ld, TotLockInstr = %8ld\n", (unsigned long) current->totalLockCycle, (unsigned long) current->totalLockInstr);
	    print_stats (current);
	    //    general_stat(current->id);
	    //MTA_printInfo (current->id);
	    current->running = 0;
	    current->freeze = 1;
	    printf ("freeze by WAIT: %d\n", current->id);
	    /* For the time being do'nt flush the thread */
	    //numcontexts--;
	    regs->regs_R[MD_REG_V0] = 0;
	    regs->regs_R[MD_REG_A3] = 0;
	    thecontexts[current->parent]->num_child--;
	}
	else
	{
	    if (current->num_child == 0)
	    {
		fprintf (stderr, "***************Thread %d is halting**************\n", current->id);
		fprintf (stderr, "TotBarCycle = %10ld, TotBarInstr = %10ld\n", (unsigned long) current->totalBarrierCycle, (unsigned long) current->totalBarrierInstr);
		fprintf (stderr, "TotLockCycle = %10ld, TotLockInstr = %10ld\n", (unsigned long) current->totalLockCycle, (unsigned long) current->totalLockInstr);
		/* Need to correct this */
		print_stats (current);
		//      general_stat(current->id);
		MTA_printInfo (current->id);
		regs->regs_R[MD_REG_V0] = 1;
		regs->regs_R[MD_REG_A3] = 0;
	    }
	    else
	    {
		regs->regs_R[MD_REG_V0] = -1;
		regs->regs_R[MD_REG_A3] = 0;
	    }

	}
	break;
//--------------------------------------------
    case SS_SYS_rt_sigprocmask:
      // not implemented yet
      regs->regs_R[MD_REG_V0] = 0;
      break;
//--------------------------------------------
    case SS_SYS_kill:
      // not implemented yet
      regs->regs_R[MD_REG_V0] = 0;
      break;

//--------------------------------------------
    case OSF_SYS_getthreadid:
	{
//              regs->regs_R[MD_REG_V0] = current->id;
	regs->regs_R[MD_REG_V0] = current->actualid;
	regs->regs_R[MD_REG_A3] = 0;
	}
	break;
//--------------------------------------------
    case OSF_SYS_STATS:
	if(collect_stats == 0)
	{
#ifndef PARALLEL_EMUL
	    collect_stats = regs->regs_R[MD_REG_A0];
#endif
#ifdef	COLLECT_STAT_STARTUP
            current->startReached  = regs->regs_R[MD_REG_A0];
#endif
			if (collect_stats == 1)
			{
			printf("Initialization over: timing simulation can start now\n");
			fprintf(stderr,"Initialization over: timing simulation can start now\n");
				for (cnt = 0; cnt < numcontexts; cnt++)
				thecontexts[cnt]->start_cycle = sim_cycle;
			flushImpStats = 1;
			}
		}
		else if (collect_stats == 1)
		{
			collect_stats = regs->regs_R[MD_REG_A0];
	#ifdef	COLLECT_STAT_STARTUP
				current->startReached  = regs->regs_R[MD_REG_A0];
	#endif
			if(collect_stats == 0)
			{
			printf("Computation over: timing simulation can stop now\n");
			fprintf(stderr,"Computation over: timing simulation can stop now\n");
			for (cnt = 0; cnt < numcontexts; cnt++)
				thecontexts[cnt]->finish_cycle = sim_cycle;
			for (cnt = 0; cnt < numcontexts; cnt++)
				{
				printf ("free freeze\n");
				thecontexts[cnt]->freeze = 0;
				thecontexts[cnt]->running = 1;
				thecontexts[cnt]->waitForSTLC = 0;
				flushWriteBuffer (cnt);
			}
			timeToReturn = 1;
			}
		}
		else
			panic ("collect_stats is invalid");
		regs->regs_R[MD_REG_V0] = 0;
		regs->regs_R[MD_REG_A3] = 0;
	break;
//--------------------------------------------
	 case OSF_SYS_BARRIER_STATS:
	{
		collect_barrier_stats_own[current->id] = regs->regs_R[MD_REG_A0];

		if (collect_barrier_stats_own[current->id] == 1)
		{
			collect_barrier_stats[current->id] = collect_barrier_stats_own[current->id];
			printf("Entering barrier %d\n", current->id);
			if(current->fastfwd_done == 0)
				current->fastfwd_done = 1;
			fflush(outFile);            // flush recoded PC
			fflush(stdout);
			current->numInstrBarrier = current->numInsn;
			current->numCycleBarrier = sim_cycle;
		}
		else if (collect_barrier_stats_own[current->id] == 0)
		{
			collect_barrier_stats[current->id] = collect_barrier_stats_own[current->id];
			printf("Leaving barrier %d\n", current->id);
			fflush(stdout);
			current->totalBarrierInstr += (current->numInsn - current->numInstrBarrier);
			current->totalBarrierCycle += (sim_cycle - current->numCycleBarrier);
		}
		else if(collect_barrier_stats_own[current->id] == 2)
			collect_barrier_release = 2;
		else
			barrier_addr = regs->regs_R[MD_REG_A0];
			
	/*	else if(collect_barrier_stats_own[current->id] >= 100 && collect_barrier_stats_own[current->id]<1000)
			printf("generate current %d\n", collect_barrier_stats_own[current->id]-100);
		else 
			printf("current %d\n", collect_barrier_stats_own[current->id]-1000);
	*/
		regs->regs_R[MD_REG_V0] = 0;
		regs->regs_R[MD_REG_A3] = 0;
	}
	break;
//--------------------------------------------
    case OSF_SYS_QUEISCE:
	{
#ifdef	COLLECT_STAT_STARTUP
			current->barrierReached = 1;
#endif

	/*		fprintf(stderr, "Thread %d is sleeping on %llx\n", current->id, regs->regs_R[MD_REG_A0]);
			fflush(stdout);*/
		if (!current->masterid)
		{
			current->running = 0;
			current->freeze = 1;
			sleepCount[current->id]++;

			//              printf("freeze by QUEISCE: %d\t%lld\t", current->id, freezeCounter);

			fflush (stdout);
			quiesceAddrStruct[current->id].address = regs->regs_R[MD_REG_A0];
			quiesceAddrStruct[current->id].threadid = current->id;
			current->sleptAt = sim_cycle;
			freezeCounter++;
		}
		regs->regs_R[MD_REG_V0] = 0;
		regs->regs_R[MD_REG_A3] = 0;
	}
	break;
//--------------------------------------------
    case OSF_SYS_LOCK_STATS:
	{
		collect_lock_stats[current->id] = regs->regs_R[MD_REG_A0];
		if (regs->regs_R[MD_REG_A0] == 1)
		{
			current->numInstrLock = current->numInsn;
			current->numCycleLock = sim_cycle;
			if(LockInitPC == 0)
			{
				LockInitPC = current->regs.regs_PC;
				printf("initialized pc for lock is %llx\n", LockInitPC);
			}
		}
		else if (regs->regs_R[MD_REG_A0] == 0)
		{
			TotalLocks++;
			current->totalLockInstr += (current->numInsn - current->numInstrLock);
			current->totalLockCycle += (sim_cycle - current->numCycleLock);
			lock_waiting[current->id] = 0;
		}
		regs->regs_R[MD_REG_V0] = 0;
		regs->regs_R[MD_REG_A3] = 0;
	}
	break;

//--------------------------------------------
    case OSF_SYS_BARRIER_INSTR:
	{
#ifdef	COLLECT_STAT_STARTUP
			current->barrierReached = 2;
#endif
#ifndef PARALLEL_EMUL
		if(collect_stats)
#endif
			TotalBarriers ++;
		collect_barrier_release = 0;
		if (startTakingStatistics == 1)
			currentPhase = 7;
		startTakingStatistics = 1;
		regs->regs_R[MD_REG_V0] = 0;
		regs->regs_R[MD_REG_A3] = 0;
		int i=0;
	}
	break;
//--------------------------------------------
    case OSF_SYS_fork:
	for (cnt = 0; cnt < numcontexts; cnt++)
	    flushWriteBuffer (cnt);
#if defined(MTA)
	/* Check for total number of threads 
	 * This need to be changed when total number of threads 
	 * can be more than the number of context supported */
	if (numcontexts >= MAXTHREADS)
	    fatal ("Total number of threads has exceeded the limit");

	/* Allocate the context */
	fprintf (stderr, "Context %d Allocating thread ID at %lld: %d\n", current->id, (numcontexts), sim_num_insn);
	printf ("Context %d Allocating thread ID: %d at %lld\n", current->id, (numcontexts), sim_num_insn);
	fflush (stderr);
	fflush (stdout);

	newContext = thecontexts[numcontexts] = (context *) calloc (1, sizeof (context));
	if (newContext == NULL)
	{
	    fprintf (stderr, "Allocation of new threads in fork failed\n");
	    exit (1);
	}

	/* Initialize the context */
	/* Not sure why we need argv */
	newContext->argv = (char **) malloc (30 * sizeof (char *));
	if (newContext->argv == NULL)
	{
	    fprintf (stderr, "Error in allocation\n");
	    exit (1);
	}

#ifdef	COLLECT_STAT_STARTUP
	thrdPerJobCnt++;
	if (thrdPerJobCnt == THREADS_PER_JOB)
	    current->fastfwd_done = 1;
	newContext->fastfwd_done = 1;
        newContext->barrierReached = 0;
        newContext->startReached = 0;
#endif
	
#ifdef	EDA
        newContext->predQueue = NULL;
        newContext->lockStatus = 0;
        newContext->boqIndex = -1;
#endif
	for (cnt = 0; cnt < 30; cnt++)
	{
	    newContext->argv[cnt] = NULL;
	    newContext->argv[cnt] = (char *) malloc (240 * sizeof (char));
	    if (newContext->argv[cnt] == NULL)
	    {
		fprintf (stderr, "Error in allocation\n");
		exit (1);
	    }
	}
	/* Allocate others */
	newContext->sim_progout = (char *) malloc (240);
	if (newContext->sim_progout == NULL)
	{
	    fprintf (stderr, "Error in allocation\n");
	    exit (1);
	}
	newContext->sim_progin = (char *) malloc (240);
	if (newContext->sim_progin == NULL)
	{
	    fprintf (stderr, "Error in allocation\n");
	    exit (1);
	}
	/* Let us first reset various parameter
	 * Can either copy from parent thread or
	 * set new value later on */
	newContext->argc = 0;
	newContext->sim_inputfd = 0;
	newContext->sim_progfd = NULL;
	/* Fast forward count is not relevant for child thread */
	newContext->fastfwd_count = 0;

	newContext->id = numcontexts;
	newContext->masterid = current->id;

	newContext->parent = current->id;

	newContext->active_this_cycle = 1;

	newContext->num_child = 0;
	current->num_child++;
	current->num_child_active++;

/*******************************/

	collectStatBarrier++;
	collectStatStop[newContext->id] = 1;

/******************************/


	newContext->actualid = ++threadForked[current->id];

	if (threadForked[current->id] == (THREADS_PER_JOB-1))
	    collectStatStop[current->id] = 1;

	/* Argument file remains same for child thread */
	newContext->argfile = current->argfile;
	memcpy (newContext->sim_progout, current->sim_progout, 240);
	memcpy (newContext->sim_progin, current->sim_progin, 240);
	/* Output file also remains same. It is the responsibility of 
	 * the application to differentiate the output from different
	 * threads */


	newContext->sim_progfd = current->sim_progfd;
	//      newContext->sim_progfd = fopen(strcat(current->sim_progout, "1"), "w");
	/* Input File remains same */
	newContext->sim_inputfd = current->sim_inputfd;

	/* I should not need fname. To check, set fname to null */
	newContext->fname = NULL;
	for (cnt = 0; cnt < 30; cnt++)
	{
	    if (current->argv[cnt])
	    {
		memcpy (newContext->argv[cnt], current->argv[cnt], 240);
	    }
	}
	newContext->argc = current->argc;

	/* Initialize the registers
	 * Copy the file from the parent thread */
	memcpy (&newContext->regs, &current->regs, sizeof (struct regs_t));
	/* Change the thread id */
	newContext->regs.threadid = numcontexts;

#if PROCESS_MODEL
	/* For the process model, we need to create a duplicate of mem */
	newContext->mem = (struct mem_t *) calloc (sizeof (struct mem_t), 1);
	newContext->mem->threadid = numcontexts;
	newContext->mem->page_count = current->mem->page_count;
	newContext->mem->ptab_misses = current->mem->ptab_misses;
	newContext->mem->ptab_accesses = current->mem->ptab_accesses;

	/* name is not a useful term. So just point it to the original
	 * name */
	newContext->mem->name = current->mem->name;
	for (cnt = 0; cnt < MEM_PTAB_SIZE; cnt++)
	{
	    if (current->mem->ptab[cnt] != NULL)
	    {
		/* Allocate first page table in the bucket */
		newContext->mem->ptab[cnt] = (struct mem_pte_t *) calloc (sizeof (struct mem_pte_t), 1);
		newContext->mem->ptab[cnt]->tag = current->mem->ptab[cnt]->tag;
		/* Allocate and copy page */
		newContext->mem->ptab[cnt]->page = (byte_t *) calloc (MD_PAGE_SIZE, 1);
		memcpy (newContext->mem->ptab[cnt]->page, current->mem->ptab[cnt]->page, MD_PAGE_SIZE);
		/* Allocate other page tables in the same bucket */
		tmpMemPtrS = current->mem->ptab[cnt]->next;
		tmpMemPtrD = newContext->mem->ptab[cnt];
		while (tmpMemPtrS != NULL)
		{
		    tmpMemPtr = (struct mem_pte_t *) calloc (sizeof (struct mem_pte_t), 1);
		    tmpMemPtr->tag = tmpMemPtrS->tag;
		    tmpMemPtr->page = (byte_t *) calloc (MD_PAGE_SIZE, 1);
		    tmpMemPtr->next = NULL;
		    memcpy (tmpMemPtr->page, tmpMemPtrS->page, MD_PAGE_SIZE);
		    tmpMemPtrD->next = tmpMemPtr;
		    tmpMemPtrD = tmpMemPtr;
		    tmpMemPtrS = tmpMemPtrS->next;
		}
	    }
	}
#else
	/* Memory remains Same. So just point to the memory space
	 * of the parent thread */
	newContext->mem = current->mem;
#endif
	/* Thread id in the mem structure remains as that of the parent id */

	newContext->ld_target_big_endian = current->ld_target_big_endian;

	newContext->ld_text_base = current->ld_text_base;
	newContext->ld_text_size = current->ld_text_size;
	newContext->ld_prog_entry = current->ld_prog_entry;
	newContext->ld_data_base = current->ld_data_base;
	newContext->ld_data_size = current->ld_data_size;
	newContext->regs.regs_R[MD_REG_GP] = current->regs.regs_R[MD_REG_GP];

	//              newContext->mem = current->mem;
	newContext->sim_swap_bytes = current->sim_swap_bytes;
	newContext->sim_swap_words = current->sim_swap_words;

	/* Set the stack. The maximum size of stack is assumed
	 * to be 0x80000(STACKSIZE). The stack is allocated above 
	 * text segment. The adress at which the stack can be allocated
	 * is kept in stack_base */


#if PROCESS_MODEL
	newContext->ld_stack_base = current->stack_base;
	newContext->stack_base = current->stack_base;
#else
	newContext->ld_stack_base = current->stack_base;
	current->stack_base = current->stack_base - STACKSIZE;
	newContext->stack_base = current->stack_base;
	/* Copy the stack from the parent to the child */
	/* Need to use mem_access function */
	/* Copy the data into the simulator memory */
	p = calloc (STACKSIZE, sizeof (char));
	mem_bcopy (mem_access, current->mem, Read, current->ld_stack_base - STACKSIZE, p, STACKSIZE, current->id);
	mem_bcopy (mem_access, current->mem, Write, newContext->ld_stack_base - STACKSIZE, p, STACKSIZE, current->id);
#endif

	/* Need to make sure that following three are being
	 * initialized properly */
	newContext->ld_stack_size = current->ld_stack_size;
	newContext->ld_environ_base = newContext->ld_environ_base - STACKSIZE;  // bug? @fanglei
	newContext->ld_brk_point = current->ld_brk_point;
#if PROCESS_MODEL
	newContext->regs.regs_R[MD_REG_SP] = current->regs.regs_R[MD_REG_SP];
#else
	newContext->regs.regs_R[MD_REG_SP] = current->regs.regs_R[MD_REG_SP] - current->ld_stack_base + newContext->ld_stack_base;
#endif
	newContext->regs.regs_PC = current->regs.regs_PC;
	//      newContext->ld_stack_min = newContext->regs.regs_R[MD_REG_SP];
	newContext->ld_stack_min = newContext->ld_stack_base;

	/* Just to make sure that EIO interfaces are not being used */
	newContext->sim_eio_fname = NULL;
	newContext->sim_chkpt_fname = NULL;
	newContext->sim_eio_fd = NULL;
	newContext->ld_prog_fname = NULL;


	/* Initialize various counters */
	newContext->rename_access = 0;
	newContext->rob1_access = 0;
	newContext->rob2_access = 0;
	newContext->sim_num_insn = 0;
	newContext->sim_total_insn = 0;
	newContext->fetch_num_insn = 0;
	newContext->fetch_total_insn = 0;
	newContext->sim_num_refs = 0;
	newContext->sim_total_refs = 0;
	newContext->sim_num_loads = 0;
	newContext->sim_total_loads = 0;
	newContext->sim_num_branches = 0;
	newContext->sim_total_branches = 0;
	newContext->sim_invalid_addrs = 0;
	newContext->inst_seq = 0;
	newContext->ptrace_seq = 0;
	newContext->ruu_fetch_issue_delay = 0;
	newContext->wait_for_fetch = 0;
#ifdef	CACHE_MISS_STAT
	newContext->spec_rdb_miss_count = 0;
	newContext->spec_wrb_miss_count = 0;
	newContext->non_spec_rdb_miss_count = 0;
	newContext->non_spec_wrb_miss_count = 0;
	newContext->inst_miss_count = 0;
	newContext->load_miss_count = 0;
	newContext->store_miss_count = 0;
#endif

	newContext->fetch_num_thrd = 0;
	newContext->iissueq_thrd = 0;
	newContext->fissueq_thrd = 0;
	newContext->icount_thrd = 0;
	newContext->last_op.next = NULL;
	newContext->last_op.rs = NULL;
	newContext->last_op.tag = 0;
	/* Not sure what should be this value */
	newContext->fetch_redirected = FALSE;
	newContext->bucket_free_list = NULL;
	newContext->start_cycle = sim_cycle;
	newContext->stallThread = 0;
	newContext->waitFor = 0;
	newContext->WBFull = 0;
	newContext->waitForBranchResolve = 0;
	newContext->NRLocalHitsLoad = 0;
	newContext->NRRemoteHitsLoad = 0;
	newContext->NRMissLoad = 0;
	newContext->NRLocalHitsStore = 0;
	newContext->NRRemoteHitsStore = 0;
	newContext->NRMissStore = 0;
	newContext->present = 0;
//              for(cnt = 0; cnt < L0BUFFERSIZE; cnt++)
//              {
//                  newContext->L0_Buffer[cnt].addr = 0;
//                  newContext->L0_Buffer[cnt].valid = 0;
//                  newContext->L0_Buffer[cnt].cntr = 0;
//              }

/*		for(cnt = 0; cnt < DATALOCATORSIZE; cnt++)
		{
		    newContext->DataLocator[cnt].addr = 0;
		    newContext->DataLocator[cnt].where = -1;
		    newContext->DataLocator[cnt].valid = 0;
		    
		}*/
	newContext->DataLocatorHit = 0;
	newContext->DataLocatorMiss = 0;


	newContext->spec_mode = current->spec_mode;
	/* System Call is always made in non-speculative mode.
	 * Therefore store_htable would be null.
	 * Also need not create bucker_free_list. This is done in spec_mem_access
	 * Also register bitmasks are not required */
	BITMAP_CLEAR_MAP (newContext->use_spec_R, R_BMAP_SZ);
	BITMAP_CLEAR_MAP (newContext->use_spec_F, F_BMAP_SZ);
	BITMAP_CLEAR_MAP (newContext->use_spec_C, C_BMAP_SZ);
	for (cnt = 0; cnt < STORE_HASH_SIZE; cnt++)
	{
	    newContext->store_htable[cnt] = NULL;
	}

	for (cnt = 0; cnt < MD_TOTAL_REGS; cnt++)
	{
	    newContext->create_vector[cnt] = CVLINK_NULL;
	    newContext->create_vector_rt[cnt] = 0;
	    newContext->spec_create_vector[cnt] = CVLINK_NULL;
	    newContext->spec_create_vector_rt[cnt] = 0;
	}

	BITMAP_CLEAR_MAP (newContext->use_spec_cv, CV_BMAP_SZ);

	/* Allocate the RUU */
	newContext->RUU = calloc (RUU_size, sizeof (struct RUU_station));
	/* Initialize each RUU entry */
	for (cnt = 0; cnt < RUU_size; cnt++)
	    newContext->RUU[cnt].index = cnt;
	newContext->RUU_num = 0;
	newContext->RUU_head = newContext->RUU_tail = 0;
	newContext->RUU_count_thrd = 0;
	newContext->RUU_fcount_thrd = 0;

	newContext->numDL1CacheAccess = 0;
	newContext->numLocalHits = 0;
	newContext->numRemoteHits = 0;
	newContext->numMemAccess = 0;
	newContext->numReadCacheAccess = 0;
	newContext->numWriteCacheAccess = 0;
	newContext->numInvalidations = 0;
	newContext->numDL1Hits = 0;
	newContext->numDL1Misses = 0;
	newContext->numInsn = 0;
	newContext->numInstrBarrier = 0;
	newContext->numCycleBarrier = 0;
	newContext->totalBarrierInstr = 0;
	newContext->totalBarrierCycle = 0;

	newContext->TotalnumDL1CacheAccess = 0;
	newContext->TotalnumLocalHits = 0;
	newContext->TotalnumRemoteHits = 0;
	newContext->TotalnumMemAccess = 0;
	newContext->TotalnumReadCacheAccess = 0;
	newContext->TotalnumWriteCacheAccess = 0;
	newContext->TotalnumInvalidations = 0;
	newContext->TotalnumDL1Hits = 0;
	newContext->TotalnumDL1Misses = 0;
	newContext->TotalnumInsn = 0;
	newContext->freeze = 0;
	newContext->waitForSTLC = 0;
	newContext->sleptAt = 0;



#if defined(TEMPDEBUG)
	printf ("Current->RUU_num = %d\n", current->RUU_num);
#endif
	newContext->finish_cycle = 0;
	newContext->running = 1;
	newContext->fetch_regs_PC = current->fetch_regs_PC;
	newContext->fetch_pred_PC = current->fetch_pred_PC;
	newContext->pred_PC = current->pred_PC;
	newContext->recover_PC = current->recover_PC;
	newContext->start_cycle = sim_cycle;


	newContext->fetch_pred_PC = current->regs.regs_NPC;
#ifdef DO_COMPLETE_FASTFWD
	newContext->regs.regs_PC += sizeof (md_inst_t);
	newContext->regs.regs_NPC = current->regs.regs_NPC + sizeof (md_inst_t);
#endif


	thecontexts[numcontexts] = newContext;
	numcontexts++;

	//      current->num_child++;
	for (cnt = 0; cnt < numcontexts; cnt++)
	{
	    priority[cnt] = cnt;
	    key[cnt] = 0;
	}

	for (cnt = 0; cnt < numcontexts; cnt++)
	    cache_flush (cache_dl1[cnt], sim_cycle);

	steer_init ();
	regs->regs_R[MD_REG_V0] = 0;
	newContext->regs.regs_R[MD_REG_V0] = newContext->actualid;
	regs->regs_R[MD_REG_A3] = 0;
	newContext->regs.regs_R[MD_REG_A3] = 0;
	regs->regs_R[MD_REG_A4] = 0;
	newContext->regs.regs_R[MD_REG_A4] = 0;
#endif
	ruu_init (newContext->id);
	lsq_init (newContext->id);
	fetch_init (newContext->id);
	cv_init (newContext->id);

	break;

/**************************************************/






    case SS_SYS_exit:
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf, /* exitcode + fudge */regs->regs_R[4]+1);
      break;
	case SS_SYS_exit_group:
      /* exit jumps to the target set in main() */
      longjmp(sim_exit_buf, /* exitcode + fudge */regs->regs_R[4]+1);
      break;



    case SS_SYS_read:
      {
	char *buf;

	/* allocate same-sized input buffer in host memory */
	if (!(buf = (char *)calloc(/*nbytes*/regs->regs_R[6], sizeof(char))))
	  fatal("out of memory in SYS_read");

	/* read data from file */
	/*nread*/regs->regs_R[2] =
	  read(/*fd*/regs->regs_R[4], buf, /*nbytes*/regs->regs_R[6]);

	/* check for error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* copy results back into host memory */
	mem_bcopy(mem_fn, mem,
		  Write, /*buf*/regs->regs_R[5],
		  buf, /*nread*/regs->regs_R[2],  current->id);

	/* done with input buffer */
	free(buf);
      }
      break;

    case SS_SYS_write:
      {
	char *buf,c;
	//printf("IN SS_SYS_write\n");
	/* allocate same-sized output buffer in host memory */
	if (!(buf = (char *)calloc(/*nbytes*/regs->regs_R[6], sizeof(char))))
	  fatal("out of memory in SYS_write");
	
	/* copy inputs into host memory */
	mem_bcopy(mem_fn, mem,
		  Read, /*buf*/regs->regs_R[5],
		  buf, /*nbytes*/regs->regs_R[6], current->id);

	/* write data to file */
	if (sim_progfd && MD_OUTPUT_SYSCALL(regs))
	  {
		
	    /* redirect program output to file */

	    /*nwritten*/regs->regs_R[2] =
	      fwrite(buf, 1, /*nbytes*/regs->regs_R[6], sim_progfd);
	  }
	else
	  {
	    /* perform program output request */
		
	    regs->regs_R[2] = write(regs->regs_R[4], buf, regs->regs_R[6]);
	
	  }

	/* check for an error condition */
	if (regs->regs_R[2] == regs->regs_R[6])
	  /*result*/regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* done with output buffer */
	free(buf);
      }
      break;

    case SS_SYS_open:
      {
	char buf[MAXBUFSIZE];
	unsigned int i;
	int ss_flags = regs->regs_R[5], local_flags = 0;

	/* translate open(2) flags */
	for (i=0; i<SS_NFLAGS; i++)
	  {
	    if (ss_flags & ss_flag_table[i].ss_flag)
	      {
		ss_flags &= ~ss_flag_table[i].ss_flag;
		local_flags |= ss_flag_table[i].local_flag;
	      }
	  }
	/* any target flags left? */
	if (ss_flags != 0)
	  ;//fatal("syscall: open: cannot decode flags: 0x%08x", ss_flags);

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	/* open the file */
	/*fd*/regs->regs_R[2] =
	  open(buf, local_flags, /*mode*/regs->regs_R[6]);
	
	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_close:
      /* don't close stdin, stdout, or stderr as this messes up sim logs */
      if (/*fd*/regs->regs_R[4] == 0
	  || /*fd*/regs->regs_R[4] == 1
	  || /*fd*/regs->regs_R[4] == 2)
	{
	  regs->regs_R[7] = 0;
	  break;
	}

      /* close the file */
      regs->regs_R[2] = close(/*fd*/regs->regs_R[4]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_creat:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	/* create the file */
	/*fd*/regs->regs_R[2] = creat(buf, /*mode*/regs->regs_R[5]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_unlink:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	/* delete the file */
	/*result*/regs->regs_R[2] = unlink(buf);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;
	case SS_SYS_time:
	{
		time_t times;
		times = time(regs->regs_R[4]);
		if(times != (time_t)(-1))
		{
			/* got an error, return details */
			regs->regs_R[2] = times;
			regs->regs_R[7] = 0;
		}
		else
		{
			/* got an error, return details */
			regs->regs_R[2] = errno;
			regs->regs_R[7] = 1;
		}
	}
	break;
    case SS_SYS_chdir:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	/* change the working directory */
	/*result*/regs->regs_R[2] = chdir(buf);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_chmod:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	/* chmod the file */
	/*result*/regs->regs_R[2] = chmod(buf, /*mode*/regs->regs_R[5]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_chown:
#ifdef _MSC_VER
      warn("syscall chown() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	/* chown the file */
	/*result*/regs->regs_R[2] = chown(buf, /*owner*/regs->regs_R[5],
				    /*group*/regs->regs_R[6]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
#endif /* _MSC_VER */
      break;

    case SS_SYS_brk:
      {
	md_addr_t addr;

	/* round the new heap pointer to the its page boundary */
	addr = ROUND_UP(/*base*/regs->regs_R[4], MD_PAGE_SIZE);
	printf("in brk @ 0x%x ld_brk_point=0x%x\n",regs->regs_R[4],current->ld_brk_point);
	/* check whether heap area has merged with stack area */
	if (addr >= current->ld_brk_point && addr < (md_addr_t)regs->regs_R[29])
	  {
	    regs->regs_R[2] = addr;
	    regs->regs_R[7] = 0;
	    current->ld_brk_point = addr;
	  }
	else if(addr == 0)
	{
		regs->regs_R[2] = current->ld_brk_point;
		regs->regs_R[7] = 0;
	}
	else
	  {
	    /* out of address space, indicate error */
	    regs->regs_R[2] = ENOMEM;
	    regs->regs_R[7] = 1;
	  }
      }
      break;
    case SS_SYS_rt_sigaction:
    {
  //  	int result, signo = regs->regs_R[4];
   // 	struct sigaction *act = regs->regs_R[5];
   // 	struct sigaction *oact = regs->regs_R[6];
   // 	printf("signo=%d, act=%x, ocat=%x\n", signo, act, oact);
   // 	result = sigaction(signo,act,oact);

    //	regs->regs_R[2] = result;
        /* check for an error condition */
        regs->regs_R[2] =0;
        regs->regs_R[7] = 0;
    }
    	break;
	case SS_SYS_llseek:

    case SS_SYS_lseek:
      /* seek into file */
      regs->regs_R[2] =
	lseek(/*fd*/regs->regs_R[4],
	      /*off*/regs->regs_R[5], /*dir*/regs->regs_R[6]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_getpid:
      /* get the simulator process id */
      /*result*/regs->regs_R[2] = getpid();

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

    case SS_SYS_getuid:
	 /*first result*/regs->regs_R[2] = getuid();
      /* check for an error condition */
      if (regs->regs_R[2] == (qword_t)-1)
	  /* got an error, return details */
	    regs->regs_R[2] = -errno;
      break;
	
	case SS_SYS_geteuid:

            /*first result*/regs->regs_R[2] = geteuid();

      /* check for an error condition */
      if (regs->regs_R[2] == (qword_t)-1)
	  /* got an error, return details */
	    regs->regs_R[2] = -errno;

      break;
	  
    case SS_SYS_access:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[4], buf, current->id);

	/* check access on the file */
	/*result*/regs->regs_R[2] = access(buf, /*mode*/regs->regs_R[5]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_stat:
    case SS_SYS_lstat:
      {
	char buf[MAXBUFSIZE];

#ifdef _MSC_VER
	struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fName*/regs->regs_R[4], buf, current->id);

	/* stat() the file */
	if (syscode == SS_SYS_stat)
	  /*result*/regs->regs_R[2] = stat(buf, &sbuf);
	else /* syscode == SS_SYS_lstat */
	  {
#ifdef _MSC_VER
	    warn("syscall lstat() not yet implemented for MSC...");
	    regs->regs_R[7] = 0;
	    break;
#else /* !_MSC_VER */
	    /*result*/regs->regs_R[2] = lstat(buf, &sbuf);
#endif /* _MSC_VER */
	  }

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* translate from host stat structure to target format */
	// ss_sbuf.ss_st_dev = MD_SWAPH(sbuf.st_dev);
	// ss_sbuf.ss_pad = 0;
	// ss_sbuf.ss_st_ino = MD_SWAPW(sbuf.st_ino);
	// ss_sbuf.ss_st_mode = MD_SWAPH(sbuf.st_mode);
	// ss_sbuf.ss_st_nlink = MD_SWAPH(sbuf.st_nlink);
	// ss_sbuf.ss_st_uid = MD_SWAPH(sbuf.st_uid);
	// ss_sbuf.ss_st_gid = MD_SWAPH(sbuf.st_gid);
	// ss_sbuf.ss_st_rdev = MD_SWAPH(sbuf.st_rdev);
	// ss_sbuf.ss_pad1 = 0;
	// ss_sbuf.ss_st_size = MD_SWAPW(sbuf.st_size);
	// ss_sbuf.ss_st_atime = MD_SWAPW(sbuf.st_atime);
	// ss_sbuf.ss_st_spare1 = 0;
	// ss_sbuf.ss_st_mtime = MD_SWAPW(sbuf.st_mtime);
	// ss_sbuf.ss_st_spare2 = 0;
	// ss_sbuf.ss_st_ctime = MD_SWAPW(sbuf.st_ctime);
	// ss_sbuf.ss_st_spare3 = 0;
// #ifndef _MSC_VER
	// ss_sbuf.ss_st_blksize = MD_SWAPW(sbuf.st_blksize);
	// ss_sbuf.ss_st_blocks = MD_SWAPW(sbuf.st_blocks);
// #endif /* !_MSC_VER */
	// ss_sbuf.ss_st_gennum = 0;
	// ss_sbuf.ss_st_spare4 = 0;

	/* copy stat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[5],
		  &sbuf, sizeof(struct stat), current->id);
      }
      break;
	case SS_SYS_uname:
	{
		struct utsname uts;
		regs->regs_R[2]=uname(&uts);
		if(regs->regs_R[2]>=0)
		{
			regs->regs_R[7] = 0;
			mem_bcopy(mem_fn, mem, Write, /*uname*/regs->regs_R[5],
				&uts, sizeof(struct utsname), current->id);
		}
		else
			regs->regs_R[7] = 1;
	}
	break;
	case SS_SYS_mprotect:
	{
		
		if (mprotect(regs->regs_R[4] , regs->regs_R[5],
                regs->regs_R[6]) == -1)
			{
				fatal("mprotect");
			}
		else
			regs->regs_R[7] = 0;

	}
	break;
	
    case SS_SYS_dup:
      /* dup() the file descriptor */
      /*fd*/regs->regs_R[2] = dup(/*fd*/regs->regs_R[4]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;

#ifndef _MSC_VER
    case SS_SYS_pipe:
      {
	int fd[2];

	/* copy pipe descriptors to host memory */;
	mem_bcopy(mem_fn, mem, Read, /*fd's*/regs->regs_R[4], fd, sizeof(fd), current->id);

	/* create a pipe */
	/*result*/regs->regs_R[7] = pipe(fd);

	/* copy descriptor results to result registers */
	/*pipe1*/regs->regs_R[2] = fd[0];
	/*pipe 2*/regs->regs_R[3] = fd[1];

	/* check for an error condition */
	if (regs->regs_R[7] == -1)
	  {
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;
#endif
	case SS_SYS_times:
      {
		struct linux_tms tms;
		struct tms ltms;
		clock_t result;

		result = times(&ltms);
		tms.tms_utime = ltms.tms_utime;
		tms.tms_stime = ltms.tms_stime;
		tms.tms_cutime = ltms.tms_cutime;
		tms.tms_cstime = ltms.tms_cstime;

		mem_bcopy(mem_fn, mem, Write,
			  /* buf */regs->regs_R[4],
				  &tms, sizeof(struct linux_tms), current->id);

			if (result != (qword_t)-1)
			  regs->regs_R[2] = result;
			else
			  regs->regs_R[2] = 1; /* got an error, return details */
      }
      break;
	
    case SS_SYS_getgid:
      /* get current group id */
      /*first result*/regs->regs_R[2] = getgid();

      /* check for an error condition */
      if (regs->regs_R[2] == (qword_t)-1)
	  /* got an error, return details */
	    regs->regs_R[2] = -errno;
      break;
	case SS_SYS_getegid:
      /* get current group id */
      /*first result*/regs->regs_R[2] = getegid();

      /* check for an error condition */
      if (regs->regs_R[2] == (qword_t)-1)
	  /* got an error, return details */
	    regs->regs_R[2] = -errno;
      break;
    case SS_SYS_ioctl:
      {
	char buf[NUM_IOCTL_BYTES];
	int local_req = 0;

	/* convert target ioctl() request to host ioctl() request values */
	switch (/*req*/regs->regs_R[5]) {
#ifdef TIOCGETP
	case SS_IOCTL_TIOCGETP:
	  local_req = TIOCGETP;
	  break;
#endif
#ifdef TIOCSETP
	case SS_IOCTL_TIOCSETP:
	  local_req = TIOCSETP;
	  break;
#endif
#ifdef TIOCGETP
	case SS_IOCTL_TCGETP:
	  local_req = TIOCGETP;
	  break;
#endif
#ifdef TCGETA
	case SS_IOCTL_TCGETA:
	  local_req = TCGETA;
	  break;
#endif
#ifdef TIOCGLTC
	case SS_IOCTL_TIOCGLTC:
	  local_req = TIOCGLTC;
	  break;
#endif
#ifdef TIOCSLTC
	case SS_IOCTL_TIOCSLTC:
	  local_req = TIOCSLTC;
	  break;
#endif
#ifdef TIOCGWINSZ
	case SS_IOCTL_TIOCGWINSZ:
	  local_req = TIOCGWINSZ;
	  break;
#endif
#ifdef TCSETAW
	case SS_IOCTL_TCSETAW:
	  local_req = TCSETAW;
	  break;
#endif
#ifdef TIOCGETC
	case SS_IOCTL_TIOCGETC:
	  local_req = TIOCGETC;
	  break;
#endif
#ifdef TIOCSETC
	case SS_IOCTL_TIOCSETC:
	  local_req = TIOCSETC;
	  break;
#endif
#ifdef TIOCLBIC
	case SS_IOCTL_TIOCLBIC:
	  local_req = TIOCLBIC;
	  break;
#endif
#ifdef TIOCLBIS
	case SS_IOCTL_TIOCLBIS:
	  local_req = TIOCLBIS;
	  break;
#endif
#ifdef TIOCLGET
	case SS_IOCTL_TIOCLGET:
	  local_req = TIOCLGET;
	  break;
#endif
#ifdef TIOCLSET
	case SS_IOCTL_TIOCLSET:
	  local_req = TIOCLSET;
	  break;
#endif
	}

#if !defined(TIOCGETP) && (defined(linux) || defined(__CYGWIN32__))
        if (!local_req && /*req*/regs->regs_R[5] == SS_IOCTL_TIOCGETP)
          {
            struct termios lbuf;
            struct ss_sgttyb buf;

            /* result */regs->regs_R[2] =
                          tcgetattr(/* fd */(int)regs->regs_R[4], &lbuf);

            /* translate results */
            buf.sg_ispeed = lbuf.c_ispeed;
            buf.sg_ospeed = lbuf.c_ospeed;
            buf.sg_erase = lbuf.c_cc[VERASE];
            buf.sg_kill = lbuf.c_cc[VKILL];
            buf.sg_flags = 0;   /* FIXME: this is wrong... */

            mem_bcopy(mem_fn, mem, Write,
                      /* buf */regs->regs_R[6], &buf,
                      sizeof(struct ss_sgttyb), current->id);

            if (regs->regs_R[2] != -1)
              regs->regs_R[7] = 0;
            else /* probably not a typewriter, return details */
              {
                regs->regs_R[2] = errno;
                regs->regs_R[7] = 1;
              }
          }
        else
#endif

	if (!local_req)
	  {
	    /* FIXME: could not translate the ioctl() request, just warn user
	       and ignore the request */
	    warn("syscall: ioctl: ioctl code not supported d=%d, req=%d",
		 regs->regs_R[4], regs->regs_R[5]);
	    regs->regs_R[2] = 0;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
#ifdef _MSC_VER
	    warn("syscall getgid() not yet implemented for MSC...");
	    regs->regs_R[7] = 0;
	    break;
#else /* !_MSC_VER */


	    /* ioctl() code was successfully translated to a host code */

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes to host mem */
	    if (/*argp*/regs->regs_R[6] != 0)
	      mem_bcopy(mem_fn, mem,
			Read, /*argp*/regs->regs_R[6], buf, NUM_IOCTL_BYTES, current->id);

	    /* perform the ioctl() call */
	    /*result*/regs->regs_R[2] =
	      ioctl(/*fd*/regs->regs_R[4], local_req, buf);

	    /* if arg ptr exists, copy NUM_IOCTL_BYTES bytes from host mem */
	    if (/*argp*/regs->regs_R[6] != 0)
	      mem_bcopy(mem_fn, mem, Write, regs->regs_R[6],
			buf, NUM_IOCTL_BYTES, current->id);

	    /* check for an error condition */
	    if (regs->regs_R[2] != -1)
	      regs->regs_R[7] = 0;
	    else
	      {	
		/* got an error, return details */
		regs->regs_R[2] = errno;
		regs->regs_R[7] = 1;
	      }
#endif /* _MSC_VER */
	  }
      }
      break;
	case SS_SYS_fstat64:
    case SS_SYS_fstat:
      {
	struct ss_statbuf ss_sbuf;
#ifdef _MSC_VER
	struct _stat sbuf;
#else /* !_MSC_VER */
	struct stat sbuf;
#endif /* _MSC_VER */

	/* fstat() the file */
	/*result*/regs->regs_R[2] = fstat(/*fd*/regs->regs_R[4], &sbuf);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* translate the stat structure to host format */
	ss_sbuf.ss_st_dev = MD_SWAPH(sbuf.st_dev);
	ss_sbuf.ss_pad = 0;
	ss_sbuf.ss_st_ino = MD_SWAPW(sbuf.st_ino);
	ss_sbuf.ss_st_mode = MD_SWAPH(sbuf.st_mode);
	ss_sbuf.ss_st_nlink = MD_SWAPH(sbuf.st_nlink);
	ss_sbuf.ss_st_uid = MD_SWAPH(sbuf.st_uid);
	ss_sbuf.ss_st_gid = MD_SWAPH(sbuf.st_gid);
	ss_sbuf.ss_st_rdev = MD_SWAPH(sbuf.st_rdev);
	ss_sbuf.ss_pad1 = 0;
	ss_sbuf.ss_st_size = MD_SWAPW(sbuf.st_size);
	ss_sbuf.ss_st_atime = MD_SWAPW(sbuf.st_atime);
        ss_sbuf.ss_st_spare1 = 0;
	ss_sbuf.ss_st_mtime = MD_SWAPW(sbuf.st_mtime);
        ss_sbuf.ss_st_spare2 = 0;
	ss_sbuf.ss_st_ctime = MD_SWAPW(sbuf.st_ctime);
        ss_sbuf.ss_st_spare3 = 0;
#ifndef _MSC_VER
	ss_sbuf.ss_st_blksize = MD_SWAPW(sbuf.st_blksize);
	ss_sbuf.ss_st_blocks = MD_SWAPW(sbuf.st_blocks);
#endif /* !_MSC_VER */
        ss_sbuf.ss_st_gennum = 0;
        ss_sbuf.ss_st_spare4 = 0;

	/* copy fstat() results to simulator memory */
	mem_bcopy(mem_fn, mem, Write, /*sbuf*/regs->regs_R[5],
		  &ss_sbuf, sizeof(struct ss_statbuf), current->id);
      }
      break;


    case SS_SYS_setitimer:
      /* FIXME: the sigvec system call is ignored */
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: setitimer ignored");
      break;
	case SS_SYS_mmap:
	{
		void *start, *return_val; //a0
		unsigned int length; //a1
		int prot;	//a2
		int flags, newflags =0;	//a3
		int fd;	//sp+16
		off_t offset;	//sp+20
		
		start = regs->regs_R[4];
		length = (unsigned int)regs->regs_R[5];
		prot  = regs->regs_R[6];
		flags = regs->regs_R[7];

		return_val = mmap_brk_point;
		mmap_brk_point += length;
		regs->regs_R[2] = return_val;
		regs->regs_R[7] = 0;
		printf("mmap_brk_point is %x\n", mmap_brk_point);
      }
      break;
	case SS_SYS_munmap:
	{
		int return_val;
		return_val = 0;
		regs->regs_R[2] = return_val;
		regs->regs_R[7] = 0;
	}   
	  break;
	case SS_SYS_mremap:
	{
		void *old_address, *return_val;
		int old_size, new_size;
		old_address = regs->regs_R[4];
		old_size = regs->regs_R[5];
		new_size = regs->regs_R[6];
		mmap_brk_point += (new_size - old_size);
		//old_address += (new_size - old_size);
		regs->regs_R[2] = old_address;
		regs->regs_R[7] = 0;
		printf("mmap_brk_point after remap is %x\n", mmap_brk_point);
	}
    case SS_SYS_dup2:
      /* dup2() the file descriptor */
      regs->regs_R[2] =
	dup2(/* fd1 */regs->regs_R[4], /* fd2 */regs->regs_R[5]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
	regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}
      break;
	case SS_SYS_fcntl64:
    case SS_SYS_fcntl:

      /* get fcntl() information on the file */
      regs->regs_R[2] =
	fcntl(/*fd*/regs->regs_R[4], /*cmd*/regs->regs_R[5],
	      /*arg*/regs->regs_R[6]);

      /* check for an error condition */
      if (regs->regs_R[2] != -1)
			regs->regs_R[7] = 0;
      else
	{
	  /* got an error, return details */
	  regs->regs_R[2] = errno;
	  regs->regs_R[7] = 1;
	}

      break;

    case SS_SYS_newselect:
#ifdef _MSC_VER
      warn("syscall select() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      {
	fd_set readfd, writefd, exceptfd;
	fd_set *readfdp, *writefdp, *exceptfdp;
	struct timeval timeout, *timeoutp;
	word_t param5;

	/* FIXME: swap words? */

	/* read the 5th parameter (timeout) from the stack */
	mem_bcopy(mem_fn, mem,
		  Read, regs->regs_R[29]+16, &param5, sizeof(word_t), current->id);

	/* copy read file descriptor set into host memory */
	if (/*readfd*/regs->regs_R[5] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*readfd*/regs->regs_R[5],
		      &readfd, sizeof(fd_set), current->id);
	    readfdp = &readfd;
	  }
	else
	  readfdp = NULL;

	/* copy write file descriptor set into host memory */
	if (/*writefd*/regs->regs_R[6] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*writefd*/regs->regs_R[6],
		      &writefd, sizeof(fd_set), current->id);
	    writefdp = &writefd;
	  }
	else
	  writefdp = NULL;

	/* copy exception file descriptor set into host memory */
	if (/*exceptfd*/regs->regs_R[7] != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*exceptfd*/regs->regs_R[7],
		      &exceptfd, sizeof(fd_set), current->id);
	    exceptfdp = &exceptfd;
	  }
	else
	  exceptfdp = NULL;

	/* copy timeout value into host memory */
	if (/*timeout*/param5 != 0)
	  {
	    mem_bcopy(mem_fn, mem, Read, /*timeout*/param5,
		      &timeout, sizeof(struct timeval), current->id);
	    timeoutp = &timeout;
	  }
	else
	  timeoutp = NULL;

#if defined(hpux) || defined(__hpux)
	/* select() on the specified file descriptors */
	/*result*/regs->regs_R[2] =
	  select(/*nfd*/regs->regs_R[4],
		 (int *)readfdp, (int *)writefdp, (int *)exceptfdp, timeoutp);
#else
	/* select() on the specified file descriptors */
	/*result*/regs->regs_R[2] =
	  select(/*nfd*/regs->regs_R[4],
		 readfdp, writefdp, exceptfdp, timeoutp);
#endif

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, return details */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* copy read file descriptor set to target memory */
	if (/*readfd*/regs->regs_R[5] != 0)
	  mem_bcopy(mem_fn, mem, Write, /*readfd*/regs->regs_R[5],
		    &readfd, sizeof(fd_set), current->id);

	/* copy write file descriptor set to target memory */
	if (/*writefd*/regs->regs_R[6] != 0)
	  mem_bcopy(mem_fn, mem, Write, /*writefd*/regs->regs_R[6],
		    &writefd, sizeof(fd_set), current->id);

	/* copy exception file descriptor set to target memory */
	if (/*exceptfd*/regs->regs_R[7] != 0)
	  mem_bcopy(mem_fn, mem, Write, /*exceptfd*/regs->regs_R[7],
		    &exceptfd, sizeof(fd_set), current->id);

	/* copy timeout value result to target memory */
	if (/* timeout */param5 != 0)
	  mem_bcopy(mem_fn, mem, Write, /*timeout*/param5,
		    &timeout, sizeof(struct timeval), current->id);
      }
#endif
      break;

   /* case SS_SYS_sigvec:
      
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: sigvec ignored");
      break;

    case SS_SYS_sigblock:

      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: sigblock ignored");
      break;

    case SS_SYS_sigsetmask:
      
      regs->regs_R[2] = regs->regs_R[7] = 0;
      warn("syscall: sigsetmask ignored");
      break;*/



    case SS_SYS_gettimeofday:
#ifdef _MSC_VER
      warn("syscall gettimeofday() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* _MSC_VER */
      {
	struct ss_timeval ss_tv;
	struct timeval tv, *tvp;
	struct ss_timezone ss_tz;
	struct timezone tz, *tzp;

	if (/*timeval*/regs->regs_R[4] != 0)
	  {
	    /* copy timeval into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeval*/regs->regs_R[4],
		      &ss_tv, sizeof(struct ss_timeval), current->id);

	    /* convert target timeval structure to host format */
	    tv.tv_sec = MD_SWAPW(ss_tv.ss_tv_sec);
	    tv.tv_usec = MD_SWAPW(ss_tv.ss_tv_usec);
	    tvp = &tv;
	  }
	else
	  tvp = NULL;

	if (/*timezone*/regs->regs_R[5] != 0)
	  {
	    /* copy timezone into host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timezone*/regs->regs_R[5],
		      &ss_tz, sizeof(struct ss_timezone), current->id);

	    /* convert target timezone structure to host format */
	    tz.tz_minuteswest = MD_SWAPW(ss_tz.ss_tz_minuteswest);
	    tz.tz_dsttime = MD_SWAPW(ss_tz.ss_tz_dsttime);
	    tzp = &tz;
	  }
	else
	  tzp = NULL;

	/* get time of day */
	/*result*/regs->regs_R[2] = gettimeofday(tvp, tzp);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate result */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	if (/*timeval*/regs->regs_R[4] != 0)
	  {
	    /* convert host timeval structure to target format */
	    ss_tv.ss_tv_sec = MD_SWAPW(tv.tv_sec);
	    ss_tv.ss_tv_usec = MD_SWAPW(tv.tv_usec);

	    /* copy timeval to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timeval*/regs->regs_R[4],
		      &ss_tv, sizeof(struct ss_timeval), current->id);
	  }

	if (/*timezone*/regs->regs_R[5] != 0)
	  {
	    /* convert host timezone structure to target format */
	    ss_tz.ss_tz_minuteswest = MD_SWAPW(tz.tz_minuteswest);
	    ss_tz.ss_tz_dsttime = MD_SWAPW(tz.tz_dsttime);

	    /* copy timezone to target memory */
	    mem_bcopy(mem_fn, mem, Write, /*timezone*/regs->regs_R[5],
		      &ss_tz, sizeof(struct ss_timezone), current->id);
	  }
      }
#endif /* !_MSC_VER */
      break;

    case SS_SYS_getrusage:
#if defined(__svr4__) || defined(__USLC__) || defined(hpux) || defined(__hpux) || defined(_AIX)
      {
	struct tms tms_buf;
	struct ss_rusage rusage;

	/* get user and system times */
	if (times(&tms_buf) != -1)
	  {
	    /* no error */
	    regs->regs_R[2] = 0;
	    regs->regs_R[7] = 0;
	  }
	else
	  {
	    /* got an error, indicate result */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* initialize target rusage result structure */
#if defined(__svr4__)
	memset(&rusage, '\0', sizeof(struct ss_rusage));
#else /* !defined(__svr4__) */
	bzero(&rusage, sizeof(struct ss_rusage));
#endif

	/* convert from host rusage structure to target format */
	rusage.ss_ru_utime.ss_tv_sec = tms_buf.tms_utime/CLK_TCK;
	rusage.ss_ru_utime.ss_tv_sec = MD_SWAPW(rusage.ss_ru_utime.ss_tv_sec);
	rusage.ss_ru_utime.ss_tv_usec = 0;
	rusage.ss_ru_stime.ss_tv_sec = tms_buf.tms_stime/CLK_TCK;
	rusage.ss_ru_stime.ss_tv_sec = MD_SWAPW(rusage.ss_ru_stime.ss_tv_sec);
	rusage.ss_ru_stime.ss_tv_usec = 0;

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[5],
		  &rusage, sizeof(struct ss_rusage), current->id);
      }
#elif defined(__unix__) || defined(unix)
      {
	struct rusage local_rusage;
	struct ss_rusage rusage;

	/* get rusage information */
	/*result*/regs->regs_R[2] =
	  getrusage(/*who*/regs->regs_R[4], &local_rusage);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate result */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* convert from host rusage structure to target format */
	rusage.ss_ru_utime.ss_tv_sec = local_rusage.ru_utime.tv_sec;
	rusage.ss_ru_utime.ss_tv_usec = local_rusage.ru_utime.tv_usec;
	rusage.ss_ru_utime.ss_tv_sec = MD_SWAPW(local_rusage.ru_utime.tv_sec);
	rusage.ss_ru_utime.ss_tv_usec =
	  MD_SWAPW(local_rusage.ru_utime.tv_usec);
	rusage.ss_ru_stime.ss_tv_sec = local_rusage.ru_stime.tv_sec;
	rusage.ss_ru_stime.ss_tv_usec = local_rusage.ru_stime.tv_usec;
	rusage.ss_ru_stime.ss_tv_sec =
	  MD_SWAPW(local_rusage.ru_stime.tv_sec);
	rusage.ss_ru_stime.ss_tv_usec =
	  MD_SWAPW(local_rusage.ru_stime.tv_usec);
	rusage.ss_ru_maxrss = MD_SWAPW(local_rusage.ru_maxrss);
	rusage.ss_ru_ixrss = MD_SWAPW(local_rusage.ru_ixrss);
	rusage.ss_ru_idrss = MD_SWAPW(local_rusage.ru_idrss);
	rusage.ss_ru_isrss = MD_SWAPW(local_rusage.ru_isrss);
	rusage.ss_ru_minflt = MD_SWAPW(local_rusage.ru_minflt);
	rusage.ss_ru_majflt = MD_SWAPW(local_rusage.ru_majflt);
	rusage.ss_ru_nswap = MD_SWAPW(local_rusage.ru_nswap);
	rusage.ss_ru_inblock = MD_SWAPW(local_rusage.ru_inblock);
	rusage.ss_ru_oublock = MD_SWAPW(local_rusage.ru_oublock);
	rusage.ss_ru_msgsnd = MD_SWAPW(local_rusage.ru_msgsnd);
	rusage.ss_ru_msgrcv = MD_SWAPW(local_rusage.ru_msgrcv);
	rusage.ss_ru_nsignals = MD_SWAPW(local_rusage.ru_nsignals);
	rusage.ss_ru_nvcsw = MD_SWAPW(local_rusage.ru_nvcsw);
	rusage.ss_ru_nivcsw = MD_SWAPW(local_rusage.ru_nivcsw);

	/* copy rusage results into target memory */
	mem_bcopy(mem_fn, mem, Write, /*rusage*/regs->regs_R[5],
		  &rusage, sizeof(struct ss_rusage), current->id);
      }
#elif defined(__CYGWIN32__) || defined(_MSC_VER)
	    warn("syscall: called getrusage()\n");
            regs->regs_R[7] = 0;
#else
#error No getrusage() implementation!
#endif
      break;

    case SS_SYS_writev:
#ifdef _MSC_VER
      warn("syscall writev() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#else /* !_MSC_VER */
      {
	int i;
	char *buf;
	struct iovec *iov;

	/* allocate host side I/O vectors */
	iov =
	  (struct iovec *)malloc(/*iovcnt*/regs->regs_R[6]
				 * sizeof(struct iovec));
	if (!iov)
	  fatal("out of virtual memory in SYS_writev");

	/* copy target side pointer data into host side vector */
	mem_bcopy(mem_fn, mem, Read, /*iov*/regs->regs_R[5],
		  iov, /*iovcnt*/regs->regs_R[6] * sizeof(struct iovec), current->id);

	/* copy target side I/O vector buffers to host memory */
	for (i=0; i < /*iovcnt*/regs->regs_R[6]; i++)
	  {
	    iov[i].iov_base = (char *)MD_SWAPW((unsigned)iov[i].iov_base);
	    iov[i].iov_len = MD_SWAPW(iov[i].iov_len);
	    if (iov[i].iov_base != NULL)
	      {
		buf = (char *)calloc(iov[i].iov_len, sizeof(char));
		if (!buf)
		  fatal("out of virtual memory in SYS_writev");
		mem_bcopy(mem_fn, mem, Read, (md_addr_t)iov[i].iov_base,
			  buf, iov[i].iov_len, current->id);
		iov[i].iov_base = buf;
	      }
	  }

	/* perform the vector'ed write */
	/*result*/regs->regs_R[2] =
	  writev(/*fd*/regs->regs_R[4], iov, /*iovcnt*/regs->regs_R[6]);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate results */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* free all the allocated memory */
	for (i=0; i < /*iovcnt*/regs->regs_R[6]; i++)
	  {
	    if (iov[i].iov_base)
	      {
		free(iov[i].iov_base);
		iov[i].iov_base = NULL;
	      }
	  }
	free(iov);
      }
#endif /* !_MSC_VER */
      break;

    case SS_SYS_utimes:
      {
	char buf[MAXBUFSIZE];

	/* copy filename to host memory */
	mem_strcpy(mem_fn, mem, Read, /*fname*/regs->regs_R[4], buf, current->id);

	if (/*timeval*/regs->regs_R[5] == 0)
	  {
#if defined(hpux) || defined(__hpux) || defined(linux)
	    /* no utimes() in hpux, use utime() instead */
	    /*result*/regs->regs_R[2] = utime(buf, NULL);
#elif defined(_MSC_VER)
	    /* no utimes() in MSC, use utime() instead */
	    /*result*/regs->regs_R[2] = utime(buf, NULL);
#elif defined(__svr4__) || defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /*result*/regs->regs_R[2] = utimes(buf, NULL);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes()\n");
#else
#error No utimes() implementation!
#endif
	  }
	else
	  {
	    struct ss_timeval ss_tval[2];
#ifndef _MSC_VER
	    struct timeval tval[2];
#endif /* !_MSC_VER */

	    /* copy timeval structure to host memory */
	    mem_bcopy(mem_fn, mem, Read, /*timeout*/regs->regs_R[5],
		      ss_tval, 2*sizeof(struct ss_timeval), current->id);

#ifndef _MSC_VER
	    /* convert timeval structure to host format */
	    tval[0].tv_sec = MD_SWAPW(ss_tval[0].ss_tv_sec);
	    tval[0].tv_usec = MD_SWAPW(ss_tval[0].ss_tv_usec);
	    tval[1].tv_sec = MD_SWAPW(ss_tval[1].ss_tv_sec);
	    tval[1].tv_usec = MD_SWAPW(ss_tval[1].ss_tv_usec);
#endif /* !_MSC_VER */

#if defined(hpux) || defined(__hpux) || defined(__svr4__)
	    /* no utimes() in hpux, use utime() instead */
	    {
	      struct utimbuf ubuf;

	      ubuf.actime = tval[0].tv_sec;
	      ubuf.modtime = tval[1].tv_sec;

	      /* result */regs->regs_R[2] = utime(buf, &ubuf);
	    }
#elif defined(_MSC_VER)
	    /* no utimes() in MSC, use utime() instead */
	    {
	      struct _utimbuf ubuf;

	      ubuf.actime = ss_tval[0].ss_tv_sec;
	      ubuf.modtime = ss_tval[1].ss_tv_sec;

	      /* result */regs->regs_R[2] = utime(buf, &ubuf);
	    }
#elif defined(__USLC__) || defined(unix) || defined(_AIX) || defined(__alpha)
	    /* result */regs->regs_R[2] = utimes(buf, tval);
#elif defined(__CYGWIN32__)
	    warn("syscall: called utimes()\n");
#else
#error No utimes() implementation!
#endif
	  }

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate results */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }
      }
      break;

    case SS_SYS_getrlimit:
    case SS_SYS_setrlimit:
#ifdef _MSC_VER
      warn("syscall get/setrlimit() not yet implemented for MSC...");
      regs->regs_R[7] = 0;
#elif defined(__CYGWIN32__)
      warn("syscall: called get/setrlimit()\n");
      regs->regs_R[7] = 0;
#else
      {
	/* FIXME: check this..., was: struct rlimit ss_rl; */
	struct ss_rlimit ss_rl;
	struct rlimit rl;

	/* copy rlimit structure to host memory */
	mem_bcopy(mem_fn, mem, Read, /*rlimit*/regs->regs_R[5],
		  &ss_rl, sizeof(struct ss_rlimit), current->id);

	/* convert rlimit structure to host format */
	rl.rlim_cur = MD_SWAPW(ss_rl.ss_rlim_cur);
	rl.rlim_max = MD_SWAPW(ss_rl.ss_rlim_max);

	/* get rlimit information */
	if (syscode == SS_SYS_getrlimit)
	  /*result*/regs->regs_R[2] = getrlimit(regs->regs_R[4], &rl);
	else /* syscode == SS_SYS_setrlimit */
	  /*result*/regs->regs_R[2] = setrlimit(regs->regs_R[4], &rl);

	/* check for an error condition */
	if (regs->regs_R[2] != -1)
	  regs->regs_R[7] = 0;
	else
	  {
	    /* got an error, indicate results */
	    regs->regs_R[2] = errno;
	    regs->regs_R[7] = 1;
	  }

	/* convert rlimit structure to target format */
	ss_rl.ss_rlim_cur = MD_SWAPW(rl.rlim_cur);
	ss_rl.ss_rlim_max = MD_SWAPW(rl.rlim_max);

	/* copy rlimit structure to target memory */
	mem_bcopy(mem_fn, mem, Write, /*rlimit*/regs->regs_R[5],
		  &ss_rl, sizeof(struct ss_rlimit), current->id);
      }
#endif
      break;
    default:
      panic("invalid/unimplemented system call encountered, code %d", syscode);
    }

#endif /* MD_CROSS_ENDIAN */

}
