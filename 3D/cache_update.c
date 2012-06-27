/*
 * cache.c - cache module routines
 *
 * This file is a part of the SimpleScalar tool suite written by
 * Todd M. Austin as a part of the Multiscalar Research Project.
 *  
 * The tool suite is currently maintained by Doug Burger and Todd M. Austin.
 * 
 * Copyright (C) 1994, 1995, 1996, 1997, 1998 by Todd M. Austin
 *
 * This source file is distributed "as is" in the hope that it will be
 * useful.  The tool set comes with no warranty, and no author or
 * distributor accepts any responsibility for the consequences of its
 * use. 
 * 
 * Everyone is granted permission to copy, modify and redistribute
 * this tool set under the following conditions:
 * 
 *    This source code is distributed for non-commercial use only.
 *    Please contact the maintainer for restrictions applying to 
 *    commercial use.
 *
 *    Permission is granted to anyone to make or distribute copies
 *    of this source code, either as received or modified, in any
 *    medium, provided that all copyright notices, permission and
 *    nonwarranty notices are preserved, and that the distributor
 *    grants the recipient permission for further redistribution as
 *    permitted by this document.
 *
 *    Permission is granted to distribute this file in compiled
 *    or executable form under the same conditions that apply for
 *    source code, provided that either:
 *
 *    A. it is accompanied by the corresponding machine-readable
 *       source code,
 *    B. it is accompanied by a written offer, with no time limit,
 *       to give anyone a machine-readable copy of the corresponding
 *       source code in return for reimbursement of the cost of
 *       distribution.  This written offer must permit verbatim
 *       duplication by anyone, or
 *    C. it is distributed by someone who received only the
 *       executable form, and is accompanied by a copy of the
 *       written offer of source code that they received concurrently.
 *
 * In other words, you are welcome to use, share and improve this
 * source file.  You are forbidden to forbid anyone else to use, share
 * and improve what you give them.
 *
 * INTERNET: dburger@cs.wisc.edu
 * US Mail:  1210 W. Dayton Street, Madison, WI 53706
 *
 * $Id: cache_update.c,v 1.1.1.1 2011/03/29 01:04:34 huq Exp $
 *
 * $Log: cache_update.c,v $
 * Revision 1.1.1.1  2011/03/29 01:04:34  huq
 * setup project CMP
 *
 * Revision 1.11  2005/09/08 21:28:20  garg
 * Full Fledged Slip-Stream Processor with stores
 *
 * Revision 1.10  2005/08/19 13:18:20  garg
 * Final Idle Slip-Stream Version
 *
 * Revision 1.9  2005/06/27 22:23:42  garg
 * *** empty log message ***
 *
 * Revision 1.8  2005/06/27 17:24:55  garg
 *
 * Cache Warmup support added during FastForwarding.
 *
 * Revision 1.7  2005/06/10 17:24:14  garg
 *
 * Cache Stats seperated per thread
 *
 * Revision 1.6  2005/05/19 17:56:41  garg
 * Some stats
 *
 * Revision 1.5  2005/05/13 13:31:13  garg
 * Indented code
 *
 * Revision 1.4  2005/05/12 21:12:30  garg
 * Still debugging: Recovery control for speculative thread due to faults added
 *
 * Revision 1.3  2005/05/08 22:59:26  garg
 * Dispatch for lead slipstream thread (only speculative emulation) implemented.
 *
 * Revision 1.2  2005/05/06 17:49:51  garg
 * *** empty log message ***
 *
 * Revision 1.1.1.1  2005/05/06 12:40:20  garg
 * ALI SMT Cluster
 *
 * Revision 1.5  1998/08/27 08:02:01  taustin
 * implemented host interface description in host.h
 * added target interface support
 * implemented a more portable random() interface
 * fixed cache writeback stats for cache flushes
 *
 * Revision 1.4  1997/03/11  01:08:30  taustin
 * updated copyright
 * long/int tweaks made for ALPHA target support
 * double-word interfaces removed
 *
 * Revision 1.3  1997/01/06  15:56:20  taustin
 * comments updated
 * fixed writeback bug when balloc == FALSE
 * strdup() changed to mystrdup()
 * cache_reg_stats() now works with stats package
 * cp->writebacks stat added to cache
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "cache.h"

#include "smt.h"
#include "context.h"

#ifdef HOTLEAKAGE
	/* Leakage: includes */
#include "cache_leak_ctrl.h"
#include "power.h"



extern int toto;


/* Leakage: some globals */
static int global_tick = 0;	/* if a global tick happened this cycle */
static int n_resets = 0;	/* no. of local counter resets in this cycle    */
static int local_width;		/* width of local counters in bits      */
static int global_width;	/* width of global counters in bits     */
static int n_lines;		/* no. of lines in the decayed_cache    */
static int tag_size;		/* tag + status bits' size in bits      */
static int tag_array_size;	/* total tag + status array size - bytes        */
static int v_addr_size;		/* no bits in virtual address   */
static int cache_size;		/* decayed_cache size - bytes   */

static double global_access_power;	/* power per access of global counter   */
static double global_af;	/* activity factor for global counter   */
static double local_access_power;	/* power per access of local counter    */
static double local_af;		/* activity factor for local counter    */
static double local_reset_power;	/* power per rest of local counter              */

/*********************************************************************************
 *********************************************************************************/
extern int sample_interval;
extern int b_decay_enabled;
extern int b_decay_profile_enabled;


static counter_t sum_real_miss;
static counter_t sum_decay_caused_miss;
static counter_t sum_real_decay_caused_miss;
static counter_t sum_false_decay_caused_miss;
static counter_t induced_decay_misses;	/* misses due to decay */
static counter_t induced_wbacks;	/* extra writebacks due to decay */
static counter_t real_decay_misses;	/* real misses to decayed lines */

#ifndef	CACHE_CLEANUP
	//#define CACHE_CLEANUP
#endif

#ifdef     CACHE_CLEANUP
	extern int ygstRobId;
	extern int ygstRobExId;
#endif

#if defined(cache_decay)
#define MAX_LIFE						100
#define MAX_USAGE						100
#define MAX_INTERVAL					100
#define MAX_INTERVAL_FINE				400


extern md_addr_t cur_pc;

/* usage profile */
static struct stat_stat_t *n_read_prof;	/* profile the # of usage of value */
static struct stat_stat_t *n_access_prof;	/* profile the # of usage of value */

static counter_t n_profiled_blocks;	/* total # of blocks profiled */

static counter_t sum_access;	/* average access = sum_access/n_profiled_blocks */
static counter_t sum_live_time;	/* average live time length = sum_live_time/n_profiled_blocks */
static counter_t sum_dead_time;	/* average dead time length = sum_dead_time/n_profiled_blocks */

static counter_t n_decay, n_valid_decay, n_invalid_decay;

#if (decayed_cache == cache_dl2)
#define ACCESS_INTERVAL_STEP_FINE		1000
#define ACCESS_INTERVAL_STEP			10000
#define ACCESS_INTERVAL_STEP_COARSE		100000
#define DEAD_TIME_STEP					10000
#define DEAD_TIME_STEP_COARSE			100000
#else
#define ACCESS_INTERVAL_STEP_FINE		10
#define ACCESS_INTERVAL_STEP			100
#define ACCESS_INTERVAL_STEP_COARSE		1000
#define DEAD_TIME_STEP					100
#define DEAD_TIME_STEP_COARSE			1000
#endif

static struct stat_stat_t *access_interval_prof;	/* profile access interval */
static struct stat_stat_t *access_interval_prof_coarse;	/* coarse profile access interval */
static struct stat_stat_t *access_interval_prof_fine;	/* coarse profile access interval */
static counter_t sum_access_interval;
static counter_t num_access_interval;


static struct stat_stat_t *valid_ratio_prof;	/* ratio of valid cache line */
static struct stat_stat_t *dirty_ratio_prof;	/* ratio of dirty cache line */

/* life time profile */
static struct stat_stat_t *whole_life_prof;	/* time_replaced - time_first_access */

#define WHOLE_LIFE_STEP	100000	/* time for each dead time step */

static struct stat_stat_t *live_time_prof;	/* time_last_access - time_first_access */

#define LIVE_TIME_STEP	50000	/* time for each live time step */

static struct stat_stat_t *dead_time_prof;	/* time_replaced - time_last_access */
static struct stat_stat_t *dead_time_prof_coarse;	/* time_replaced - time_last_access */

static struct stat_stat_t *dirty_time_prof;	/* time_last_access - time_dirty */

#define DIRTY_time_STEP	100	/* time for each dirty time step */

static counter_t sum_dirty_time;

#define MAX_WRITEBACK		100
static struct stat_stat_t *n_writeback_prof;	/* time_last_access - time_dirty */

extern int global_counter_max;
extern int local_counter_max;
extern int fast_counter;
extern int slow_counter;

static counter_t counter_update;

/* level 1 instruction cache, entry level instruction cache */
extern struct cache_t *cache_il1;

/* level 1 instruction cache */
extern struct cache_t *cache_il2;

/* instruction TLB */
extern struct cache_t *itlb;

/* data TLB */
extern struct cache_t *dtlb;

/* Cache */
extern struct cache_t *cache_dl0;

#endif

#endif //HOTLEAKAGE

extern struct cache_t *cache_dl1;
extern struct cache_t *cache_dl2;
extern struct cache_t *cache_dl0;

int l0HitFlag;
int l1HitFlag;
int il1HitFlag;
int l2HitFlag;
int tlbHitFlag;
md_addr_t l2ReplaceAddr;

int dl2_prefetch_active = 0;
int dl2_prefetch_id = 0;
extern counter_t pfl2Hit;
extern counter_t pfl2SecMiss;
extern counter_t pfl2PrimMiss;

extern counter_t sim_cycle[8];
extern int core_speed[8];
extern int fastest_core;
extern int fastest_core_speed;

extern counter_t totalRecStoreLoadComm;
extern counter_t totalInstDelayStoreLoadComm;
extern int m_nLSQIdx;
extern struct RUU_station *LSQ[];         /* load/store queue */
#ifdef CFJ
	extern int sbdLeadThread;
	extern int mainLeadThread;
#endif

	/**********************************************************************/
	/* extract/reconstruct a block address */
#define CACHE_BADDR(cp, addr)	((addr) & ~(cp)->blk_mask)
#define CACHE_MK_BADDR(cp, tag, set)					\
		(((tag) << (cp)->tag_shift)|((set) << (cp)->set_shift))

#define CACHE_MK1_BADDR(cp, tag, set)					\
		((((tag) << (cp)->tag_shift)/BANKS)|((set) << (cp)->set_shift))

	/* index an array of cache blocks, non-trivial due to variable length blocks */
#define CACHE_BINDEX(cp, blks, i)					\
		((struct cache_blk_t *)(((char *)(blks)) +				\
								(i)*(sizeof(struct cache_blk_t) +		\
									 ((cp)->balloc				\
									  ? (cp)->bsize*sizeof(byte_t) : 0))))

																	 /* cache block hashing macros, this macro is used to index into a cache
																		set hash table (to find the correct block on N in an N-way cache), the
																		cache set index function is CACHE_SET, defined above */
#define CACHE_HASH(cp, key)						\
																		 (((key >> 24) ^ (key >> 16) ^ (key >> 8) ^ key) & ((cp)->hsize-1))

																	 /* cache data block accessor, type parameterized */
#define __CACHE_ACCESS(type, data, bofs)				\
																		 (*((type *)(((char *)data) + (bofs))))

																	 /* cache data block accessors, by type */
#define CACHE_DOUBLE(data, bofs)  __CACHE_ACCESS(double, data, bofs)
#define CACHE_FLOAT(data, bofs)	  __CACHE_ACCESS(float, data, bofs)
#define CACHE_WORD(data, bofs)	  __CACHE_ACCESS(unsigned int, data, bofs)
#define CACHE_HALF(data, bofs)	  __CACHE_ACCESS(unsigned short, data, bofs)
#define CACHE_BYTE(data, bofs)	  __CACHE_ACCESS(unsigned char, data, bofs)


																	 /* copy data out of a cache block to buffer indicated by argument pointer p */
#define CACHE_BCOPY(cmd, blk, bofs, p, nbytes)	\
																		 if (cmd == Read)							\
{									\
	switch (nbytes) {							\
		case 1:								\
											*((byte_t *)p) = CACHE_BYTE(&blk->data[0], bofs); break;	\
		case 2:								\
											*((half_t *)p) = CACHE_HALF(&blk->data[0], bofs); break;	\
		case 4:								\
											*((word_t *)p) = CACHE_WORD(&blk->data[0], bofs); break;	\
		default:								\
												{ /* >= 8, power of two, fits in block */			\
													int words = nbytes >> 2;					\
													while (words-- > 0)						\
													{								\
														*((word_t *)p) = CACHE_WORD(&blk->data[0], bofs);	\
														p += 4; bofs += 4;					\
													}\
												}\
	}\
}\
																	 else /* cmd == Write */						\
{									\
	switch (nbytes) {							\
		case 1:								\
											CACHE_BYTE(&blk->data[0], bofs) = *((byte_t *)p); break;	\
		case 2:								\
											CACHE_HALF(&blk->data[0], bofs) = *((half_t *)p); break;	\
		case 4:								\
											CACHE_WORD(&blk->data[0], bofs) = *((word_t *)p); break;	\
		default:								\
												{ /* >= 8, power of two, fits in block */			\
													int words = nbytes >> 2;					\
													while (words-- > 0)						\
													{								\
														CACHE_WORD(&blk->data[0], bofs) = *((word_t *)p);		\
														p += 4; bofs += 4;					\
													}\
												}\
	}\
}

																	 /* copy data out of a cache string to buffer indicated by argument pointer p */
#define CACHE_SCOPY(cmd, data, bofs, p, nbytes)	\
																		 if (cmd == Read)							\
{									\
	switch (nbytes) {							\
		case 1:								\
											*((byte_t *)p) = CACHE_BYTE(data, bofs); break;	\
		case 2:								\
											*((half_t *)p) = CACHE_HALF(data, bofs); break;	\
		case 4:								\
											*((word_t *)p) = CACHE_WORD(data, bofs); break;	\
		default:								\
												{ /* >= 8, power of two, fits in block */			\
													int words = nbytes >> 2;					\
													while (words-- > 0)						\
													{								\
														*((word_t *)p) = CACHE_WORD(data, bofs);	\
														p += 4; bofs += 4;					\
													}\
												}\
	}\
}\
																	 else /* cmd == Write */						\
{									\
	switch (nbytes) {							\
		case 1:								\
											CACHE_BYTE(data, bofs) = *((byte_t *)p); break;	\
		case 2:								\
											CACHE_HALF(data, bofs) = *((half_t *)p); break;	\
		case 4:								\
											CACHE_WORD(data, bofs) = *((word_t *)p); break;	\
		default:								\
												{ /* >= 8, power of two, fits in block */			\
													int words = nbytes >> 2;					\
													while (words-- > 0)						\
													{								\
														CACHE_WORD(data, bofs) = *((word_t *)p);		\
														p += 4; bofs += 4;					\
													}\
												}\
	}\
}
																	 /**********************************************************************/

																	 /* bound squad_t/dfloat_t to positive int */
#define BOUND_POS(N)		((int)(min2(max2(0, (N)), 2147483647)))

																	 /* Ronz' stats */

#define BANKS 16		/* Doesn't matter unless you use address-ranges.
						   Same for bank_alloc. */
																	 /*
																		int bank_alloc[CACHEPORT] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
																	  */
																	 int laten[BANKS];

																	 /* Cache access stats to compute power numbers with Dave's model */
																	 long rm1 = 0;
																	 long wm1 = 0;
																	 long rh1 = 0;
																	 long wh1 = 0;
																	 long rm2 = 0;
																	 long wm2 = 0;
																	 long rh2 = 0;
																	 long wh2 = 0;

																	 extern int activecontexts;
																	 extern int res_membank;
																	 extern int n_cache_limit_thrd[];
																	 extern int n_cache_start_thrd[];

#ifdef  LOAD_PREDICTOR
																	 extern int cache_miss;
#endif //LOAD_PREDICTOR

#ifdef CACHE_THRD_STAT
																	 extern double n_cache_miss_thrd[];
																	 extern double n_cache_access_thrd[];
																	 extern double n_cache_miss_thrash_thrd[];
#endif

																	 void insert_fillq (long, md_addr_t, int);

																	 extern int numcontexts;

																	 /* unlink BLK from the hash table bucket chain in SET */
	static void
unlink_htab_ent (struct cache_t *cp,	/* cache to update */
		struct cache_set_t *set,	/* set containing bkt chain */
		struct cache_blk_t *blk)	/* block to unlink */
{
	struct cache_blk_t *prev, *ent;
	int index = CACHE_HASH (cp, blk->tagid.tag);

	/* locate the block in the hash table bucket chain */
	for (prev = NULL, ent = set->hash[index]; ent; prev = ent, ent = ent->hash_next)
	{
		if (ent == blk)
			break;
	}
	assert (ent);

	/* unlink the block from the hash table bucket chain */
	if (!prev)
	{
		/* head of hash bucket list */
		set->hash[index] = ent->hash_next;
	}
	else
	{
		/* middle or end of hash bucket list */
		prev->hash_next = ent->hash_next;
	}
	ent->hash_next = NULL;
}

/* insert BLK onto the head of the hash table bucket chain in SET */
	static void
link_htab_ent (struct cache_t *cp,	/* cache to update */
		struct cache_set_t *set,	/* set containing bkt chain */
		struct cache_blk_t *blk)	/* block to insert */
{
	int index = CACHE_HASH (cp, blk->tagid.tag);

	/* insert block onto the head of the bucket chain */
	blk->hash_next = set->hash[index];
	set->hash[index] = blk;
}

/* insert BLK into the order way chain in SET at location WHERE */
	static void
update_way_list (struct cache_set_t *set,	/* set contained way chain */
		struct cache_blk_t *blk,	/* block to insert */
		enum list_loc_t where, int assoc)	/* insert location */
{
	/* unlink entry from the way list */
	if (!blk->way_prev && !blk->way_next)
	{
		/* only one entry in list (direct-mapped), no action */
		assert (set->way_head == blk && set->way_tail == blk);
		/* Head/Tail order already */
		return;
	}
	/* else, more than one element in the list */
	else if (!blk->way_prev)
	{
		assert (set->way_head == blk && set->way_tail != blk);
		if (where == Head)
		{
			/* already there */
			return;
		}
		/* else, move to tail */
		set->way_head = blk->way_next;
		blk->way_next->way_prev = NULL;
	}
	else if (!blk->way_next)
	{
		/* end of list (and not front of list) */
		assert (set->way_head != blk && set->way_tail == blk);
		if (where == Tail)
		{
			/* already there */
			return;
		}
		set->way_tail = blk->way_prev;
		blk->way_prev->way_next = NULL;
	}
	else
	{
		/* middle of list (and not front or end of list) */
		assert (set->way_head != blk && set->way_tail != blk);
		blk->way_prev->way_next = blk->way_next;
		blk->way_next->way_prev = blk->way_prev;
	}

	/* link BLK back into the list */
	if (where == Head)
	{
		/* link to the head of the way list */
		blk->way_next = set->way_head;
		blk->way_prev = NULL;
		set->way_head->way_prev = blk;
		set->way_head = blk;
	}
	else if (where == Tail)
	{
		/* link to the tail of the way list */
		blk->way_prev = set->way_tail;
		blk->way_next = NULL;
		set->way_tail->way_next = blk;
		set->way_tail = blk;
	}
#ifdef ALT_CACHE_MNG_POLICY
	else if (where == Mid)
	{
		int i;
		struct cache_blk_t *insblk = set->way_head;
		for(i = 0; i < (assoc/2-1); i++)
			insblk = insblk->way_next;
		blk->way_prev = insblk;
		blk->way_next = insblk->way_next;
		insblk->way_next->way_prev = blk;
		insblk->way_next = blk;
	}
#endif
	else
		panic ("bogus WHERE designator");
}

#ifdef HOTLEAKAGE
	void
clear_cache_stats (struct cache_t *cp)
{
	/* initialize cache stats */
	int i;

	for (i = 0; i < MAXTHREADS; i++)
	{
		cp->hits[i] = 0;
		cp->misses[i] = 0;
		cp->read_misses[i] = 0;
		cp->write_misses[i] = 0;
		cp->replacements[i] = 0;
		cp->writebacks[i] = 0;
		cp->invalidations[i] = 0;
	}
}				/* clear_cache_stats */
#endif //HOTLEAKAGE

/* create and initialize a general cache structure */
struct cache_t *		/* pointer to cache created */
cache_create (char *name,	/* name of the cache */
		int nsets,	/* total number of sets in cache */
		int bsize,	/* block (line) size of cache */
		int balloc,	/* allocate data space for blocks? */
		int usize,	/* size of user data to alloc w/blks */
		int assoc,	/* associativity of cache */
		enum cache_policy policy,	/* replacement policy w/in sets */
		/* block access function, see description w/in struct cache def */
		unsigned int (*blk_access_fn) (enum mem_cmd cmd, md_addr_t baddr, int bsize, struct cache_blk_t * blk, tick_t now, int threadid), unsigned int hit_latency)	/* latency in cycles for a hit */
{
	struct cache_t *cp;
	struct cache_blk_t *blk;
	int i, j, bindex;

	/* check all cache parameters */
	if (nsets <= 0)
		fatal ("cache size (in sets) `%d' must be non-zero", nsets);
	if ((nsets & (nsets - 1)) != 0)
		fatal ("cache size (in sets) `%d' is not a power of two", nsets);
	/* blocks must be at least one datum large, i.e., 8 bytes for SS */
	if (bsize < 8)
		fatal ("cache block size (in bytes) `%d' must be 8 or greater", bsize);
	if ((bsize & (bsize - 1)) != 0)
		fatal ("cache block size (in bytes) `%d' must be a power of two", bsize);
	if (usize < 0)
		fatal ("user data size (in bytes) `%d' must be a positive value", usize);
	if (assoc <= 0)
		fatal ("cache associativity `%d' must be non-zero and positive", assoc);

	if (!blk_access_fn)
		fatal ("must specify miss/replacement functions");

	/* allocate the cache structure */
	cp = (struct cache_t *) calloc (1, sizeof (struct cache_t) + (nsets - 1) * sizeof (struct cache_set_t));
	if (!cp)
		fatal ("out of virtual memory");

	/* initialize user parameters */
	cp->name = mystrdup (name);
	cp->nsets = nsets;
	cp->bsize = bsize;
	cp->balloc = balloc;
	cp->usize = usize;
	cp->assoc = assoc;
	cp->policy = policy;
	cp->hit_latency = hit_latency;

	/* miss/replacement functions */
	cp->blk_access_fn = blk_access_fn;

	/* compute derived parameters */
	cp->hsize = CACHE_HIGHLY_ASSOC (cp) ? (assoc >> 2) : 0;
	cp->blk_mask = bsize - 1;
	cp->set_shift = log_base2 (bsize);
	cp->set_mask = nsets - 1;
	cp->tag_shift = cp->set_shift + log_base2 (nsets);
	cp->tag_mask = (1 << (32 - cp->tag_shift)) - 1;
	cp->tagset_mask = ~cp->blk_mask;
	cp->bus_free = 0;
#ifdef FINE_GRAIN_INV
	cp->word_num = cp->bsize/GRANULARITY;
	cp->word_shift = log_base2(GRANULARITY);
	cp->word_mask = cp->word_num-1;
#endif

#ifdef HOTLEAKAGE
	/* print derived parameters during debug */
	debug ("%s: cp->hsize     = %d", cp->name, cp->hsize);
	debug ("%s: cp->blk_mask  = 0x%08x", cp->name, cp->blk_mask);
	debug ("%s: cp->set_shift = %d", cp->name, cp->set_shift);
	debug ("%s: cp->set_mask  = 0x%08x", cp->name, cp->set_mask);
	debug ("%s: cp->tag_shift = %d", cp->name, cp->tag_shift);
	debug ("%s: cp->tag_mask  = 0x%08x", cp->name, cp->tag_mask);
#else //HOTLEAKAGE
	/* print derived parameters during debug */
	debug ("%s: cp->hsize     = %d", cp->hsize);
	debug ("%s: cp->blk_mask  = 0x%08x", cp->blk_mask);
	debug ("%s: cp->set_shift = %d", cp->set_shift);
	debug ("%s: cp->set_mask  = 0x%08x", cp->set_mask);
	debug ("%s: cp->tag_shift = %d", cp->tag_shift);
	debug ("%s: cp->tag_mask  = 0x%08x", cp->tag_mask);
#endif //HOTLEAKAGE

	/* initialize cache stats */
	for (i = 0; i < MAXTHREADS; i++)
	{
		cp->hits[i] = 0;
		cp->misses[i] = 0;
#ifdef HOTLEAKAGE
		cp->read_misses[i] = 0;
		cp->write_misses[i] = 0;
#endif //HOTLEAKAGE
		cp->replacements[i] = 0;
		cp->writebacks[i] = 0;
		cp->invalidations[i] = 0;
	}
	cp->rdb = 0;
	cp->wrb = 0;
	cp->lastuse = 0;

	/* blow away the last block accessed */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;

	/* allocate data blocks */
	cp->data = (byte_t *) calloc (nsets * assoc, sizeof (struct cache_blk_t) + (cp->balloc ? (bsize * sizeof (byte_t)) : 0));
	if (!cp->data)
		fatal ("out of virtual memory");

	/* slice up the data blocks */
	for (bindex = 0, i = 0; i < nsets; i++)
	{
		cp->sets[i].way_head = NULL;
		cp->sets[i].way_tail = NULL;
#ifdef ALT_CACHE_MNG_POLICY
		cp->sets[i].insertionPolicy = Head;
		cp->sets[i].waitrcCounter = 0;
		cp->sets[i].rcValid = 0;
#endif
		/* get a hash table, if needed */
		if (cp->hsize)
		{
			cp->sets[i].hash = (struct cache_blk_t **) calloc (cp->hsize, sizeof (struct cache_blk_t *));
			if (!cp->sets[i].hash)
				fatal ("out of virtual memory");
		}
		/* NOTE: all the blocks in a set *must* be allocated contiguously,
		   otherwise, block accesses through SET->BLKS will fail (used
		   during random replacement selection) */
		cp->sets[i].blks = CACHE_BINDEX (cp, cp->data, bindex);

		/* link the data blocks into ordered way chain and hash table bucket
		   chains, if hash table exists */
		for (j = 0; j < assoc; j++)
		{
			/* locate next cache block */
			blk = CACHE_BINDEX (cp, cp->data, bindex);
			bindex++;

			/* invalidate new cache block */
			blk->status = 0;
			blk->tagid.tag = 0;
			blk->tagid.threadid = -1;
			blk->ready = 0;
			blk->user_data = (usize != 0 ? (byte_t *) calloc (usize, sizeof (byte_t)) : NULL);
			blk->isByteDirty = create_byte_mask(cp->bsize);

			blk->last_access_time = 0;
			blk->isStale = 0;
			blk->last_access_insn = 0;
			blk->store_not_executed = 0;
			blk->wrongLoadCauses = 0;
			blk->poison = 0;
#ifdef CACHE_CLEANUP
			blk->robId = 0;
			blk->robExId = 3;
#endif
#ifdef FINE_GRAIN_INV
			if(!strcmp(cp->name,"dl0"))
			{
				blk->wordStatus = (int *) calloc (cp->word_num, sizeof(int));
				int k;
				for(k = 0; k < cp->word_num; k++)
					blk->wordStatus[k] = 0;
			}
			else
				blk->wordStatus = NULL;
#endif
#ifdef HOTLEAKAGE
			/* cache_decay: init */
#if defined(cache_decay)
			blk->time_first_access = 0;
			blk->time_last_access = 0;
			blk->time_dirty = 0;
			blk->n_access = 0;
			blk->n_total_access = 0;
			blk->n_total_miss = 0;
			blk->n_read = 0;
			blk->n_write = 0;
			blk->frequency = 0;
			blk->local_counter_max = local_counter_max;
			blk->local_counter = blk->local_counter_max;
#endif /* defined(cache_decay) */
#endif //HOTLEAKAGE

			/* insert cache block into set hash table */
			if (cp->hsize)
				link_htab_ent (cp, &cp->sets[i], blk);

			/* insert into head of way list, order is arbitrary at this point */
			blk->way_next = cp->sets[i].way_head;
			blk->way_prev = NULL;
			if (cp->sets[i].way_head)
				cp->sets[i].way_head->way_prev = blk;
			cp->sets[i].way_head = blk;
			if (!cp->sets[i].way_tail)
				cp->sets[i].way_tail = blk;
		}
	}
	return cp;
}

/* parse policy */
	enum cache_policy		/* replacement policy enum */
cache_char2policy (char c)	/* replacement policy as a char */
{
	switch (c)
	{
		case 'l':
			return LRU;
		case 'r':
			return Random;
		case 'f':
			return FIFO;
		default:
			fatal ("bogus replacement policy, `%c'", c);
	}
}

/* print cache configuration */
	void
cache_config (struct cache_t *cp,	/* cache instance */
		FILE * stream)	/* output stream */
{
	fprintf (stream, "cache: %s: %d sets, %d byte blocks, %d bytes user data/block\n", cp->name, cp->nsets, cp->bsize, cp->usize);
	fprintf (stream, "cache: %s: %d-way, `%s' replacement policy, write-back\n", cp->name, cp->assoc, cp->policy == LRU ? "LRU" : cp->policy == Random ? "Random" : cp->policy == FIFO ? "FIFO" : (abort (), ""));
}

/* register cache stats */
	void
cache_reg_stats (struct cache_t *cp,	/* cache instance */
		struct stat_sdb_t *sdb)	/* stats database */
{
	int i;

	for (i = 0; i < numcontexts; i++)
	{
		char buf[512], buf1[512], *name;

		/* get a name for this cache */
		if (!cp->name || !cp->name[0])
			name = "<unknown>";
		else
			name = cp->name;

		sprintf (buf, "%s.hits[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of hits", &cp->hits[i], 0, NULL);
		sprintf (buf, "%s.misses[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of misses", &cp->misses[i], 0, NULL);
#ifdef HOTLEAKAGE
		sprintf (buf, "%s.read_misses[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of read misses", &cp->read_misses[i], 0, NULL);
		sprintf (buf, "%s.write_misses[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of write misses", &cp->write_misses[i], 0, NULL);
#endif //HOTLEAKAGE
		sprintf (buf, "%s.replacements[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of replacements", &cp->replacements[i], 0, NULL);
		sprintf (buf, "%s.writebacks[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of writebacks", &cp->writebacks[i], 0, NULL);
		sprintf (buf, "%s.invalidations[%d]", name, i);
		stat_reg_counter (sdb, buf, "total number of invalidations", &cp->invalidations[i], 0, NULL);
	}
}

#ifdef HOTLEAKAGE
/* update cache stats every sample_interval cycles */
	int
update_cache_stats ()
{
#if defined(cache_decay)	/* do nothing if do not decay cache */
	int i, bindex;
	struct cache_blk_t *blk;
	int n_valid, n_dirty, n_total;
	struct cache_t *cp = decayed_cache;

	n_valid = 0;
	n_dirty = 0;
	n_total = cp->assoc * cp->nsets;


	/* count the valid ratio and dirty ratio */
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
			if (blk->status & CACHE_BLK_VALID)
			{
				n_valid++;
				if (blk->status & CACHE_BLK_DIRTY)
					n_dirty++;
			}
		}
	}				/* for */
	return 0;
#endif
}				/* update_cache_stats */

	int
print_total_access ()
{
	int i, bindex;
	struct cache_blk_t *blk;
	struct cache_t *cp = decayed_cache;

	fprintf (stderr, " cache total access\n");

	for (i = 0; i < cp->nsets; i++)
	{
		for (bindex = 0; bindex < cp->assoc; bindex++)
		{
			blk = CACHE_BINDEX (cp, cp->sets[i].blks, bindex);
			fprintf (stderr, " %ld            %ld", (long) blk->n_total_access, (long) blk->n_total_miss);
		}

		fprintf (stderr, " \n");
	}

	fprintf (stderr, " \n\n\n");

	return 0;
}				/* update_cache_stats */

/* update cache decay every global_counter_max cycles */
	int
update_cache_decay ()
{
#if defined(cache_decay)
	struct cache_blk_t *blk;
	struct cache_t *cp = decayed_cache;
	int n_writeback = 0;
	int i;

	/* Leakage: locals      */
	counter_t incr_count = 0;

	/* Leakage: update stats        */
	if (!cache_leak_is_ctrlled ())
		return 0;
	global_tick = 1;

	/* glock tick arrived, update local counters */
	for (i = 0; i < cp->nsets; i++)
	{
		int k;

		for (blk = cp->sets[i].blks, k = 0; k < cp->assoc; blk++, k++)
		{

			if (!(blk->status & CACHE_BLK_DECAYED))
			{
				counter_update++;
				blk->local_counter--;

				/* perform decay here */
				if (blk->local_counter < 0)
				{


					/* decay this cache block */
					/* Leakage: block still valid if no loss of state */
					if (cache_leak_ctrl_is_state_losing ())
						blk->status &= ~CACHE_BLK_VALID;
					blk->status |= CACHE_BLK_DECAYED;
					blk->time_decayed = sim_cycle[fastest_core];
					n_decay++;

					/* Leakage: update stats        */
					/* mode switch to low   */
					mode_switch_h2l_incr ();
					incr_count++;

					/* after decay, use local counter to count how long will next miss happen. */
#if defined(counter_based_adaptive) || defined(combined_adaptive)
					blk->local_counter = blk->local_counter_max;
#endif /* defined(counter_based_adaptive) || defined(combined_adaptive) */

					/* Leakage: modification */
					/* need to write back dirty decay blocks  if state losing */
					if ((blk->status & CACHE_BLK_DIRTY) && cache_leak_ctrl_is_state_losing ())
					{
						cp->writebacks[blk->tagid.threadid]++;
						n_writeback++;
						induced_wbacks++;	/* pessimistic estimate */
						/* decayed line could be followed only by reads not resulting in extra wbacks */
						cp->blk_access_fn (Write, CACHE_MK_BADDR (cp, blk->tagid.tag, i /* set */ ), cp->bsize, blk, sim_cycle[fastest_core], blk->tagid.threadid);
					}
				}
			}
			else
			{
			}
		}			/* for blk */
	}				/* for i */

	/*
	 * Leakage: update stats
	 * leakage throughout the cache assumed uniform. Also to model
	 * the effect of settling time of leakage current, the lines
	 * are assumed to be turned off after 'switch_cycles_h2l/2'.
	 * The assumption is that settling is a linear function of time.
	 */

	low_leak_ratio_incr ((double) incr_count / (cp->nsets * cp->assoc), get_switch_cycles_h2l () / 2);

	return 0;
#endif
}				/* update_cache_decay */


	void
update_cache_block_stats_when_hit (struct cache_t *cp, struct cache_blk_t *blk, enum mem_cmd cmd)
{
	int i;
	int access_interval;

#if defined(cache_decay)
	if (cp == decayed_cache)
	{
		if (blk->time_first_access > 0)
		{
			access_interval = sim_cycle[fastest_core] - blk->time_last_access + 1;


			sum_access_interval += access_interval;
			num_access_interval += 1;

			blk->time_last_access = sim_cycle[fastest_core];
			blk->pc_last_access = cur_pc;
			blk->n_access++;
			blk->n_total_access++;
			blk->last_cmd = cmd;
			if (cmd == Read)
				blk->n_read++;
			else
				blk->n_write++;

			/* reset the counter for each cache hit */
			/* cache hit when power is on, reset the local counter */
			if (blk->local_counter != blk->local_counter_max)
			{
				blk->local_counter = blk->local_counter_max;
				counter_update++;
				/* Leakage: local counter reset */
				n_resets++;
			}
		}
		else
		{
			i = 0;
		}			/* if */

	}				/* if cp */
#endif
}				/* update_cache_block_stats_when_hit */

/* update cache block stats when we are sure the block is dead, current done when it's evicted out of the cache */
	void
update_cache_block_stats_when_miss (struct cache_t *cp, struct cache_blk_t *repl, enum mem_cmd cmd, int decay_caused_miss)
{
#if defined(cache_decay)	/* refresh profile when cache block is replaced out */
	if (cp == decayed_cache)
	{
		/* if this block has been initialized */
		if (repl->time_first_access > 0)
		{
			n_profiled_blocks++;

			sum_access += repl->n_access;
			sum_live_time += repl->time_last_access - repl->time_first_access;
			sum_dead_time += sim_cycle[fastest_core] - repl->time_last_access;


			if (repl->time_decayed)
			{
				n_invalid_decay += decay_caused_miss;
				n_valid_decay += 1 - decay_caused_miss;
			}

		}			/* if */

		/* reset cache blocks, the replaced block is now reset to be a new valid block */
		repl->time_first_access = sim_cycle[fastest_core];
		repl->first_cmd = cmd;

		repl->pc_last_access = cur_pc;
		repl->time_last_access = sim_cycle[fastest_core];
		repl->last_cmd = cmd;

		repl->time_decayed = 0;	/* init to never decayed */
		repl->n_total_miss++;

		repl->n_access = 1;
		repl->n_total_access++;
		repl->n_read = (cmd == Read ? 1 : 0);
		repl->n_write = (cmd == Write ? 1 : 0);


		/* reset the counter for each access */
		if (repl->local_counter != repl->local_counter_max)
		{
			repl->local_counter = repl->local_counter_max;
			counter_update++;
			/* Leakage: local counter reset */
			n_resets++;
		}

	}				/* if */
#endif
}				/* update_cache_block_stats_when_miss */
#endif //HOTLEAKAGE

/* print cache stats */
	void
cache_stats (struct cache_t *cp,	/* cache instance */
		FILE * stream,	/* output stream */
		int threadid)
{
	double sum = (double) (cp->hits[threadid] + cp->misses[threadid]);

	fprintf (stream, "cache: %s: %.0f hits %.0f misses %.0f repls %.0f invalidations\n", cp->name, (double) cp->hits[threadid], (double) cp->misses[threadid], (double) cp->replacements[threadid], (double) cp->invalidations[threadid]);
	fprintf (stream, "cache: %s: miss rate=%f  repl rate=%f  invalidation rate=%f\n", cp->name, (double) cp->misses[threadid] / sum, (double) (double) cp->replacements[threadid] / sum, (double) cp->invalidations[threadid] / sum);
}

#ifdef HOTLEAKAGE
extern int b_in_dispatch;
#endif //HOTLEAKAGE

	int
bank_check (md_addr_t addr, struct cache_t *cp)
{
	/*
	   int c, bnk, i, mask, set, sets_per_bank;
	   c = cluster_check(addr);
	   bnk=0;
	   for (i=0;i<c;i++) {
	   bnk += bank_alloc[i];
	   }
	   mask = (((cp->set_mask+1)*bank_alloc[c])/BANKS) - 1;
	   set =  ((addr >> cp->set_shift) & mask);
	   sets_per_bank = ((cp->set_mask+1)/BANKS);
	   bnk += set/sets_per_bank;
	   return bnk;
	 */
	return 0;
}

counter_t new_store_not_exec = 0;
counter_t prev_store_not_exec = 0;
counter_t new_sim_num_insn = 0;
counter_t prev_sim_num_insn = 0;
counter_t prev_sim_cycle = 0;
struct cache_blk_t *current_blk = NULL;

/* access a cache, perform a CMD operation on cache CP at address ADDR,
   places NBYTES of data at *P, returns latency of operation if initiated
   at NOW, places pointer to block user data in *UDATA, *P is untouched if
   cache blocks are not allocated (!CP->BALLOC), UDATA should be NULL if no
   user data is attached to blocks */
	unsigned int			/* latency of access in cycles */
cache_access (struct cache_t *cp,	/* cache to access */
		enum mem_cmd cmd,	/* access type, Read or Write */
		md_addr_t addr,	/* address of access */
		void *vp,		/* ptr to buffer for input/output */
		int nbytes,	/* number of bytes to access */
		tick_t now,	/* time of access */
		byte_t ** udata,	/* for return of user data ptr */
		md_addr_t * repl_addr,	/* for address of replaced block */
		int threadid)
{
	byte_t *p = vp;
	md_addr_t tag;
	md_addr_t set;
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *blk, *repl;
	int lat = 0;

	unsigned int repl_status = 0;
	tick_t repl_ready = 0;
	byte_t repl_data[cp->bsize];
	int port_lat = 0, now_lat = 0, port_now = 0;

	md_addr_t bank_per_thread, thrd_bank_start, thrd_word_bank;
	md_addr_t set_bank;
	md_addr_t wb_addr;
	md_addr_t tag_bank, repl_thread, repl_bank, repl_thrd_bank, repl_set_bank, repl_tag;

#ifdef CFJ
	int dup = 0;
	byte_t dup_data[cp->bsize];
	int dup_done = 0;
#endif
#ifdef HOTLEAKAGE
	int possible_real_miss = 0;
	int low_leak_penalty_flag = 0;
	int temp;
	int decay_caused_miss = FALSE;	/* TRUE if it's a decay caused miss */

	if (b_in_dispatch)
		b_in_dispatch = TRUE;
#endif //HOTLEAKAGE

	if (!strcmp (cp->name, "dl0"))
		l0HitFlag = 0;
	if (!strcmp (cp->name, "dl1"))
		l1HitFlag = 0;
	if (!strcmp (cp->name, "il1") || !strcmp (cp->name, "il0"))
		il1HitFlag = 0;
	if (!strcmp (cp->name, "ul2"))
		l2HitFlag = 0;
	if (!strcmp (cp->name, "dtlb"))
		tlbHitFlag = 0;

	l2ReplaceAddr = 0;

	tag = CACHE_TAG (cp, addr);
	set = CACHE_SET (cp, addr);

#ifdef    ICACHE_BANK_CLUSTER
	/* 
	 ** IL1 Cache Configuration for Chip MultiProcessor
	 ** 1. Cache sets are divided into number of CMPs.
	 ** 2. Number of CMPs = CLUSTERS
	 ** 3. Basic config remains same
	 */
#endif // ICACHE_BANK_CLUSTER

	/* default replacement address */
	if (repl_addr)
		*repl_addr = 0;

	/* check alignments */
	if ((nbytes & (nbytes - 1)) != 0 || (addr & (nbytes - 1)) != 0)
		fatal ("cache: access error: bad size or alignment, addr 0x%08x", addr);

	/* access must fit in cache block */
	if ((addr + nbytes - 1) > ((addr & ~cp->blk_mask) + cp->bsize - 1))
		fatal ("cache: access error: access spans block, addr 0x%08x", addr);

	/* permissions are checked on cache misses */

	if (cp->hsize)
	{
		/* higly-associativity cache, access through the per-set hash tables */
		int hindex = CACHE_HASH (cp, tag);

		for (blk = cp->sets[set].hash[hindex]; blk; blk = blk->hash_next)
		{
#ifdef HOTLEAKAGE
			if ((blk->status & CACHE_BLK_DECAYED) && cache_leak_is_ctrlled ())
				low_leak_penalty_flag = 1;
			/* Thread ID match not required for slipstream processor */
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */ )
			{
				/* Leakage: induced misses only in state losing ctrl techniques */
				if ((blk->status & CACHE_BLK_DECAYED) && cache_leak_ctrl_is_state_losing ())
				{
					decay_caused_miss = TRUE;
					induced_decay_misses++;
					break;
				}
				else if ((blk->status & CACHE_BLK_DECAYED) && (blk->status & CACHE_BLK_VALID) && cache_leak_is_ctrlled ())
				{
					/*
					 * Leakage: update stats
					 * in state preserving ctrl, mode switch to high happens
					 * on a hit to a decayed block too
					 */

					mode_switch_l2h_incr ();
					/*
					 * leakage throughout the cache assumed uniform. Also to model
					 * the effect of settling time of leakage current, the lines
					 * are assumed to be turned off after 'switch_cycles_l2h/2'.
					 * The assumption is that settling is a linear function of time.
					 */
					low_leak_ratio_dcr (1.0 / (cp->nsets * cp->assoc), get_switch_cycles_l2h () / 2);

#ifdef     CACHE_CLEANUP
					if(!strcmp(cp->name,"dl0") && blk->robExId != 3 && ((blk->robExId == ygstRobExId && blk->robId > ygstRobId) || (blk->robExId == (ygstRobExId+1)%3)))
					{
						repl = blk;
						goto  partial_cache_miss;
					}
#endif
					goto cache_hit;
				}
				else if (blk->status & CACHE_BLK_VALID)
				{
#ifdef     CACHE_CLEANUP
					if(!strcmp(cp->name,"dl0") && blk->robExId != 3 && ((blk->robExId == ygstRobExId && blk->robId > ygstRobId) || (blk->robExId == (ygstRobExId+1)%3)))
					{
						repl = blk;
						goto  partial_cache_miss;
					}
#endif
					goto cache_hit;
				}
			}
			else if (blk->status & CACHE_BLK_DECAYED)
				possible_real_miss = 1;
#else //HOTLEAKAGE
			/* ThreadID chech is not required for slipstream processor */
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */  &&
					(blk->status & CACHE_BLK_VALID))
			{
#ifdef CFJ
				if(!strcmp(cp->name,"dl0") && thecontexts[sbdLeadThread]->running && threadid == mainLeadThread && (blk->status & CACHE_BLK_DIRTY) && (blk->tagid.threadid != threadid))
				{
					dup = 1;
					continue;
				}
				if(!strcmp(cp->name,"dl0") && thecontexts[sbdLeadThread]->running && threadid == sbdLeadThread && (blk->tagid.threadid != threadid) && (blk->dup || cmd == Write))
				{
					if (cp->balloc)
					{
						byte_t *rd = &dup_data[0];
						md_addr_t rofs = 0;
						CACHE_BCOPY (Read, blk, rofs, rd, cp->bsize);
						dup_done = 1;
					}
					blk->dup = 1;
					continue;
				}
#endif
#ifdef     CACHE_CLEANUP
				if(!strcmp(cp->name,"dl0") && blk->robExId != 3 && ((blk->robExId == ygstRobExId && blk->robId > ygstRobId) || (blk->robExId == (ygstRobExId+1)%3)))
				{
					repl = blk;
					goto  partial_cache_miss;
				}
#endif
				goto cache_hit;
			}
#endif //HOTLEAKAGE
		}
	}
	else
	{
		/* low-associativity cache, linear search the way list */
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
#ifdef HOTLEAKAGE
			if ((blk->status & CACHE_BLK_DECAYED) && cache_leak_is_ctrlled ())
				low_leak_penalty_flag = 1;
			/* ThreadID chech is not required for slipstream processor */
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */ )
			{
				/* Leakage: induced misses only in state losing ctrl techniques */
				if ((blk->status & CACHE_BLK_DECAYED) && cache_leak_ctrl_is_state_losing ())
				{
					decay_caused_miss = TRUE;

					if (cp == decayed_cache)
					{
						induced_decay_misses++;
						break;
					}
				}
				else if ((blk->status & CACHE_BLK_DECAYED) && (blk->status & CACHE_BLK_VALID) && cache_leak_is_ctrlled ())
				{
					/*
					 * Leakage: update stats
					 * in state preserving ctrl, mode switch to high happens
					 * on a hit to a decayed block too
					 */

					mode_switch_l2h_incr ();
					/*
					 * leakage throughout the cache assumed uniform. Also to model
					 * the effect of settling time of leakage current, the lines
					 * are assumed to be turned off after 'switch_cycles_l2h/2'.
					 * The assumption is that settling is a linear function of time.
					 */
					low_leak_ratio_dcr (1.0 / (cp->nsets * cp->assoc), get_switch_cycles_l2h () / 2);

#ifdef     CACHE_CLEANUP
					if(!strcmp(cp->name,"dl0") && blk->robExId != 3 && ((blk->robExId == ygstRobExId && blk->robId > ygstRobId) || (blk->robExId == (ygstRobExId+1)%3)))
					{
						repl = blk;
						goto  partial_cache_miss;
					}
#endif
					goto cache_hit;
				}
				else if (blk->status & CACHE_BLK_VALID)
				{
#ifdef     CACHE_CLEANUP
					if(!strcmp(cp->name,"dl0") && blk->robExId != 3 && ((blk->robExId == ygstRobExId && blk->robId > ygstRobId) || (blk->robExId == (ygstRobExId+1)%3)))
					{
						repl = blk;
						goto  partial_cache_miss;
					}
#endif
					goto cache_hit;
				}
			}
			else if (blk->status & CACHE_BLK_DECAYED)
				possible_real_miss = 1;
#else //HOTLEAKAGE
			/* ThreadID chech is not required for slipstream processor */
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */  &&
					(blk->status & CACHE_BLK_VALID))
			{
#ifdef CFJ
				if(!strcmp(cp->name,"dl0") && thecontexts[sbdLeadThread]->running && threadid == mainLeadThread && (blk->status & CACHE_BLK_DIRTY) && (blk->tagid.threadid != threadid))
				{
					dup = 1;
					continue;
				}
				if(!strcmp(cp->name,"dl0") && thecontexts[sbdLeadThread]->running && threadid == sbdLeadThread && (blk->tagid.threadid != threadid) && (blk->dup || cmd == Write))
				{
					if (cp->balloc)
					{
						byte_t *rd = &dup_data[0];
						md_addr_t rofs = 0;
						CACHE_BCOPY (Read, blk, rofs, rd, cp->bsize);
						dup_done = 1;
					}
					blk->dup = 1;
					continue;
				}
#endif
#ifdef     CACHE_CLEANUP
				if(!strcmp(cp->name,"dl0") && blk->robExId != 3 && ((blk->robExId == ygstRobExId && blk->robId > ygstRobId) || (blk->robExId == (ygstRobExId+1)%3)))
				{
					repl = blk;
					goto  partial_cache_miss;
				}
#endif
				goto cache_hit;
			}
#endif //HOTLEAKAGE
		}
	}

#ifdef ACCURATE_CACHE_MISS_IMP
	/* Look at the live and repl queue for a hit */
	if (!strcmp (cp->name, "ul2"))
	{
		struct repl_live_t *temp = search_repl_live(CACHE_MK_BADDR (cp, tag, set));
		if(temp)
		{
			l2HitFlag = 1;
			cp->hits[threadid]++;
			/* copy data out of cache block, if block exists */
			if (cp->balloc)
			{
				CACHE_SCOPY (cmd, temp->data, bofs, p, nbytes);
			}

			if (!strcmp (cp->name, "ul2"))
			{
				if (cmd == Write)
					wh2++;
				else
					rh2++;
			}

			/* return first cycle data is available to access */
			return (int) max2 (cp->hit_latency, (temp->ready - now));
		}
	}
#endif

	/* cache block not found */
	/* **MISS** */
	extern FILE *trace_file;
	extern int isCacheAccessSpec;
	cp->misses[threadid]++;
	//if (!strcmp (cp->name, "dl1"))
	//    fprintf(trace_file,"%d\t%d\t%lld\t%lld\n", isCacheAccessSpec, threadid, CACHE_MK_BADDR (cp, tag, set), sim_cycle[0]);
#ifdef HOTLEAKAGE
	if (cmd == Write)
		cp->write_misses[threadid]++;
	else
		cp->read_misses[threadid]++;

	if (cp == decayed_cache && !decay_caused_miss && possible_real_miss)
		real_decay_misses++;
#endif //HOTLEAKAGE

#ifdef LOAD_PREDICTOR
	if (!strcmp (cp->name, "dl1"))
		cache_miss = 1;
#endif //LOAD_PREDICTOR

#ifdef CACHE_THRD_STAT
	if (!strcmp (cp->name, "dl1"))
	{
		n_cache_miss_thrd[threadid]++;
		n_cache_access_thrd[threadid]++;
	}
#endif

	/* select the appropriate block to replace, and re-link this entry to
	   the appropriate place in the way list */
	switch (cp->policy)
	{
		case LRU:
		case FIFO:
			repl = cp->sets[set].way_tail;
#ifdef HOTLEAKAGE
			/* FIXMEHZG: replacement policy: choose invalid block first, does this diff from LRU?  */
#if defined(cache_decay)
			if (b_decay_enabled)
			{
				int k, found = 0;

				for (blk = cp->sets[set].blks, k = 0; k < cp->assoc; blk++, k++)
				{
					/* invalid block has highest priority to be evicted */
					if (!(blk->status & CACHE_BLK_VALID))
					{
						repl = blk;
						found = 1;
						break;
					}
				}
				/* Leakage: if an invalid blk can't be found, find a shutdown one */
				if (!found && cache_leak_ctrl_is_state_losing ())
					for (blk = cp->sets[set].blks, k = 0; k < cp->assoc; blk++, k++)
					{
						/* invalid block has highest priority to be evicted */
						if (blk->status & CACHE_BLK_DECAYED)
						{
							repl = blk;
							break;
						}
					}
			}
#endif /* defined(cache_decay) */
#endif //HOTLEAKAGE
#ifdef ALT_CACHE_MNG_POLICY
			if(!strcmp (cp->name, "dl1"))
			{
				if(cp->sets[set].rcValid)
				{
					if(cp->sets[set].rcTag == tag)
					{
						cp->sets[set].waitrcCounter = cp->sets[set].rcCounter;
						if(cp->sets[set].rcCounter < cp->assoc/2)
							cp->sets[set].insertionPolicy = Mid;
						else if(cp->sets[set].rcCounter <= cp->assoc)
							cp->sets[set].insertionPolicy = Head;
						else
						{
							cp->sets[set].insertionPolicy = Tail;
							cp->sets[set].waitrcCounter = 0;
						}

						cp->sets[set].rcCounter = 0;
						cp->sets[set].rcValid = 0;
					}
					else
						cp->sets[set].rcCounter++;

					if(cp->sets[set].rcCounter > 8)
					{
						cp->sets[set].insertionPolicy = Tail;
						cp->sets[set].rcValid = 0;
					}
				}
				else
				{
					cp->sets[set].waitrcCounter--;
					if(cp->sets[set].waitrcCounter == 0)
					{
						cp->sets[set].rcValid = 1;
						cp->sets[set].rcTag = tag;
						cp->sets[set].rcCounter = 1;
					}
				}
				update_way_list (&cp->sets[set], repl, cp->sets[set].insertionPolicy, cp->assoc);
				if(set == 0)
				{
					fprintf(trace_file,"Mis - %d\t", set);
					fprintf(trace_file,"%d\n", tag);
				}
			}
			else
#endif
				update_way_list (&cp->sets[set], repl, Head, cp->assoc);
			break;
		case Random:
			{
				int bindex = myrand () & (cp->assoc - 1);

				repl = CACHE_BINDEX (cp, cp->sets[set].blks, bindex);
			}
			break;
		default:
			panic ("bogus replacement policy");
	}

	/* remove this block from the hash bucket chain, if hash exists */
	if (cp->hsize)
		unlink_htab_ent (cp, &cp->sets[set], repl);


#ifdef CACHE_THRD_STAT
	if (!strcmp (cp->name, "dl1"))
	{
		if ((repl->tagid.threadid != -1) && (repl->tagid.threadid != threadid) && (repl->status & CACHE_BLK_VALID))
			n_cache_miss_thrash_thrd[threadid]++;
	}
#endif //CACHE_THRD_STAT


	/* blow away the last block to hit */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;

#ifdef HOTLEAKAGE
	if (low_leak_penalty_flag == 1 && /* repl-> status & CACHE_BLK_DECAYED */ cache_leak_is_ctrlled ())
	{
		temp = get_low_leak_penalty ();
		/* latency hiding assumed */
		lat += get_low_leak_penalty ();
	}


	/* Leakage: update stats */
	/* mode switch to high happens if   block to be evicted is decayed */
	if (repl->status & CACHE_BLK_DECAYED)
	{
		mode_switch_l2h_incr ();
		/*
		 * leakage throughout the cache assumed uniform. Also to model
		 * the effect of settling time of leakage current, the lines
		 * are assumed to be turned off after 'switch_cycles_l2h/2'.
		 * The assumption is that settling is a linear function of time.
		 */
		low_leak_ratio_dcr (1.0 / (cp->nsets * cp->assoc), get_switch_cycles_l2h () / 2);
	}
#endif //HOTLEAKAGE

	lat += cp->hit_latency;

	{
		if (!strcmp (cp->name, "dl0"))
		{			/* Eviction stats */
			extern int lastTh0CommittedInsn;
			extern counter_t cntCacheEviction;
			extern counter_t cntCEStale;
			extern counter_t cntCEStaleDGd;
			extern counter_t cntCECorrDBd;

			cntCacheEviction++;
			if (repl->store_not_executed > repl->last_access_insn)
			{			/* This line does not have the most updated data */
				cntCEStale++;
				if (lastTh0CommittedInsn >= repl->store_not_executed)
					cntCEStaleDGd++;
			}
			else
			{
				if (lastTh0CommittedInsn < repl->last_access_insn)
					cntCECorrDBd++;
			}
		}
		counter_t temp_sim_num_insn = new_sim_num_insn;
		counter_t temp_store_not_exec = new_store_not_exec;

		new_sim_num_insn = repl->last_access_insn;
		new_store_not_exec = repl->store_not_executed;
		repl->wrongLoadCauses = 0;
		repl->poison = 0;
		repl->isStale = 0;
		if (cmd == Write)
		{
			repl->last_access_time = now;
			repl->last_access_insn = temp_sim_num_insn;
			repl->store_not_executed = temp_store_not_exec;
		}
	}

	/* write back replaced block data */
	if ((strcmp (cp->name, "dl0") != 0) && repl->status & CACHE_BLK_VALID)
	{
		cp->replacements[threadid]++;

		if (repl_addr)
			*repl_addr = CACHE_MK_BADDR (cp, repl->tagid.tag, set);

		if(strcmp (cp->name, "ul2") == 0)
			l2ReplaceAddr = CACHE_MK_BADDR (cp, repl->tagid.tag, set);

#ifndef ACCURATE_CACHE_MISS_IMP
		/* don't replace the block until outstanding misses are satisfied */
		lat += BOUND_POS (repl->ready - (now + lat));
#endif

#ifdef HOTLEAKAGE
		/* stall until the bus to next level of memory is available */
		lat += BOUND_POS (cp->bus_free - (now + lat));

		/* track bus resource usage */
		cp->bus_free = MAX (cp->bus_free, (now + lat)) + 1;
#endif //HOTLEAKAGE

		if ((strcmp (cp->name, "ul2") == 0))
			insert_vCache (CACHE_MK_BADDR (cp, repl->tagid.tag, set), cp->bsize, repl);

		if (repl->status & CACHE_BLK_DIRTY)
		{
			/* write back the cache block */
			cp->writebacks[threadid]++;
			/* Increment the writeback queue size */
			cp->wrb++;
			lat += cp->blk_access_fn (Write, CACHE_MK_BADDR (cp, repl->tagid.tag, set), cp->bsize, repl, now + lat, repl->tagid.threadid);
		}
	}

	/* Contending for the next level. Drain out enough writes to make space
	   for yourself and all previous reads. - Ronz */

#ifdef  NO_BUS_CONTENTION
	port_lat = 0;
#else
	port_lat = (BOUND_POS (cp->wrb - cp->wrbufsize) + (cp->rdb)) * cp->pipedelay;	/* used */
#endif
#ifdef  SIM_CYCLE_CAST_BUG
	now_lat = (int) (now - (tick_t) sim_cycle[fastest_core]);	/* used */
#else //  SIM_CYCLE_CAST_BUG
	now_lat = now - sim_cycle[fastest_core];
#endif //SIM_CYCLE_CAST_BUG
	port_now = BOUND_POS (port_lat - now_lat);	/* used */
	lat += BOUND_POS (port_now + BOUND_POS (cp->lastuse + cp->pipedelay - now) - lat);
	cp->rdb++;

#ifdef HOTLEAKAGE
	if (b_decay_profile_enabled)
		update_cache_block_stats_when_miss (cp, repl, cmd, decay_caused_miss);
#endif //HOTLEAKAGE

	/* Save replaced cache line stats */
	repl_status = repl->status;
	repl_ready = repl->ready;
	if (cp->balloc)
	{
		byte_t *rd = &repl_data[0];
		md_addr_t rofs = 0;
		CACHE_BCOPY (Read, repl, rofs, rd, cp->bsize);
	}

	/* update block tags */
	repl->tagid.tag = tag;
	repl->tagid.threadid = threadid;
	repl->status = CACHE_BLK_VALID;	/* dirty bit set on update */
	repl->addr = addr;
#ifdef CFJ
	repl->dup = dup;
#endif
#ifdef STREAM_PREFETCHER
	repl->spTag = 0;
	if (!strcmp (cp->name, "ul2") && dl2_prefetch_active)
	{
		repl->spTag = dl2_prefetch_id+1;
	}
#endif
#ifdef ALT_CACHE_MNG_POLICY
	int i;
	for(i = 0; i < 32; i++)
		repl->lastTouch[i] = 0;
#endif
#ifdef FINE_GRAIN_INV
	if(repl->wordStatus)
	{
		int k;
		for(k = 0; k < cp->word_num; k++)
			repl->wordStatus[k] = 1;
	}
#endif

#ifdef HOTLEAKAGE
	repl->status &= ~CACHE_BLK_DECAYED;	/* not decayed  */
#endif //HOTLEAKAGE

#ifdef CACHE_CLEANUP
partial_cache_miss: /* Partial cache miss */

	repl->robId = LSQ[m_nLSQIdx]->robId;
	repl->robExId = LSQ[m_nLSQIdx]->robExId;
#endif

#ifndef NO_L2CACHE_ACCESS
	/* read data block */
	lat += cp->blk_access_fn (Read, CACHE_TAGSET (cp, addr), cp->bsize, repl, now + lat, threadid);
#endif //NO_L2CACHE_ACCESS

	/* Inform the pipeline when the data is ready for L1D refill, so that
	   contention for that can be modeled  - Ronz */
	if (!strcmp (cp->name, "dl1"))
	{
		insert_fillq ((long) (now + lat), addr, threadid);
	}
#ifdef CFJ
	if (cp->balloc && dup_done)
	{
		byte_t *rd = &dup_data[0];
		md_addr_t rofs = 0;
		CACHE_BCOPY (Write, repl, rofs, rd, cp->bsize);
	}
	if (!strcmp (cp->name, "dl0") && cmd == Write && threadid == mainLeadThread)
		sbd_line_write(cp, addr, vp, nbytes, sbdLeadThread);
#endif

	/* copy data out of cache block */
	if (cp->balloc)
	{
		CACHE_BCOPY (cmd, repl, bofs, p, nbytes);
	}

	/* update dirty status */
#ifdef HOTLEAKAGE
	if (cmd == Write)
	{
		blk->status |= CACHE_BLK_DIRTY;
		blk->time_dirty = sim_cycle[fastest_core];
	}
#else //HOTLEAKAGE
	if (cmd == Write)
	{
		repl->status |= CACHE_BLK_DIRTY;
		clear_mask(repl->isByteDirty, cp->bsize);		
		add_mask(repl->isByteDirty, CACHE_BLK (cp, addr), nbytes, cp->bsize);
	}
#endif //HOTLEAKAGE
	if (cmd == Read)
	{
		repl->last_access_time = prev_sim_cycle;
		repl->last_access_insn = prev_sim_num_insn;
		repl->store_not_executed = prev_store_not_exec;
	}
	current_blk = repl;

	/* Update the access stats - Ronz */
	if (!strcmp (cp->name, "dl1"))
	{
		if (cmd == Write)
		{
			wm1++;
		}
		else
		{
			rm1++;
		}
	}
	if (!strcmp (cp->name, "ul2"))
	{
		if (cmd == Write)
		{
			wm2++;
		}
		else
		{
			rm2++;
		}
	}

	/* get user block data, if requested and it exists */
	if (udata)
		*udata = repl->user_data;

#ifdef ACCURATE_CACHE_MISS_IMP
	if(repl_status & CACHE_BLK_VALID)
	{
		lat += BOUND_POS (repl_ready - (now + lat));
		if (!strcmp (cp->name, "ul2") && repl_addr)
			insert_repl_live(*repl_addr, repl_ready, now + lat, now, &repl_data);
	}
#endif
	/* update block status */
	repl->ready = now + lat;

	/* link this entry back into the hash table */
	if (cp->hsize)
		link_htab_ent (cp, &cp->sets[set], repl);

	/* return latency of the operation */
	return lat;

	/* **HIT** */
cache_hit:			/* slow hit handler */

	if (!strcmp (cp->name, "dl0"))
		l0HitFlag = 1;
	if (!strcmp (cp->name, "dl1"))
		l1HitFlag = 1;
	if (!strcmp (cp->name, "il1") || !strcmp (cp->name, "il0"))
		il1HitFlag = 1;
	if (!strcmp (cp->name, "ul2"))
		l2HitFlag = 1;
	if (!strcmp (cp->name, "dtlb"))
		tlbHitFlag = 1;
#ifdef HOTLEAKAGE
	/* Leakage: for hits in low leak mode */

	if (blk->status & CACHE_BLK_DECAYED && cache_leak_is_ctrlled ())
	{
		blk->status &= ~CACHE_BLK_DECAYED;
		temp = get_low_leak_penalty ();
		/* latency hiding assumed */
		if (blk->ready < now + get_low_leak_penalty ())
			blk->ready = now + get_low_leak_penalty () + cp->hit_latency;
	}

	if (b_decay_profile_enabled)
		update_cache_block_stats_when_hit (cp, blk, cmd);
#endif //HOTLEAKAGE

	cp->hits[threadid]++;
#ifdef CACHE_THRD_STAT
	if (!strcmp (cp->name, "dl1"))
	{
		n_cache_access_thrd[threadid]++;
	}
#endif

#ifdef FINE_GRAIN_INV
	if(blk->wordStatus && !strcmp(cp->name,"dl0"))
	{
		extern int fineGrainHit;
		int word_blk = CACHE_WORD_BLK(cp, addr);
		if(blk->wordStatus[word_blk] == 0)
		{/* Fill this block from L1 cache */
			byte_t *line_holder = calloc (cp->bsize, sizeof (byte_t));
			byte_t *l0cache_line = line_holder;
			md_addr_t l0cache_ofs = 0;
			CACHE_BCOPY (Read, blk, l0cache_ofs, l0cache_line, cp->bsize);
			lat += cp->blk_access_fn (Read, CACHE_TAGSET (cp, addr), cp->bsize, blk, now + lat, threadid);
			l0cache_ofs = bofs & ~(GRANULARITY-1); 
			l0cache_line = line_holder + l0cache_ofs;
			CACHE_BCOPY (Read, blk, l0cache_ofs, l0cache_line, GRANULARITY);
			l0cache_ofs = 0; 
			l0cache_line = line_holder;
			CACHE_BCOPY (Write, blk, l0cache_ofs, l0cache_line, cp->bsize);
			blk->wordStatus[word_blk] = 1;
			free (line_holder);
		}
		else
		{
			int comb_status = 1;
			int k = 0;
			for(k = 0; k < cp->word_num; k++)
				comb_status = comb_status & blk->wordStatus[k];
			if(comb_status)
				fineGrainHit = 1;
		}
	}
#endif

	/* copy data out of cache block, if block exists */
	if (cp->balloc)
	{
		CACHE_BCOPY (cmd, blk, bofs, p, nbytes);
	}

#ifdef CFJ
	if (!strcmp (cp->name, "dl0") && cmd == Write && threadid == mainLeadThread)
		sbd_line_write(cp, addr, vp, nbytes, sbdLeadThread);
#endif

	/* update dirty status */
	if (cmd == Write)
	{
		blk->status |= CACHE_BLK_DIRTY;
		add_mask(blk->isByteDirty, CACHE_BLK (cp, addr), nbytes, cp->bsize);
#ifdef HOTLEAKAGE
		blk->time_dirty = sim_cycle[fastest_core];
#endif //HOTLEAKAGE
	}

	/* Update access stats - Ronz */
	if (!strcmp (cp->name, "dl1"))
	{
		if (cmd == Write)
		{
			wh1++;
		}
		else
		{
			rh1++;
		}
	}
	if (!strcmp (cp->name, "ul2"))
	{
		if (cmd == Write)
		{
			wh2++;
		}
		else
		{
			rh2++;
		}
	}

#ifdef ALT_CACHE_MNG_POLICY
	if(!strcmp (cp->name, "dl1"))
	{
		if(set == 0)
		{
			fprintf(trace_file,"Hit - %d\t", set);
			fprintf(trace_file,"%d\n", tag);
		}
	}
#endif

	/* if LRU replacement and this is not the first element of list, reorder */
	if (/*blk->way_prev &&*/ cp->policy == LRU)
	{
		int qofs = CACHE_BLK (cp, addr)/8;
		if(qofs >= 32)
			qofs = 0;
		/* move this block to head of the way (MRU) list */
#ifdef ALT_CACHE_MNG_POLICY
		if(strcmp (cp->name, "dl1") || blk->lastTouch[qofs])
#endif
		{
#ifdef ALT_CACHE_MNG_POLICY
			//blk->lastTouch[qofs] = 0;
#endif
			update_way_list (&cp->sets[set], blk, Head, cp->assoc);
		}
#ifdef ALT_CACHE_MNG_POLICY
		else
			blk->lastTouch[qofs] = 1;
#endif
	}

	if (cmd == Write)
	{
		blk->last_access_time = now;
		blk->last_access_insn = new_sim_num_insn;
		blk->store_not_executed = new_store_not_exec;
		blk->wrongLoadCauses = 0;
		blk->poison = 0;
#ifdef CACHE_CLEANUP
		blk->robId = LSQ[m_nLSQIdx]->robId;
		blk->robExId = LSQ[m_nLSQIdx]->robExId;
#endif
	}
	blk->isStale = 0;
	prev_sim_cycle = blk->last_access_time;
	prev_sim_num_insn = blk->last_access_insn;
	prev_store_not_exec = blk->store_not_executed;
	current_blk = blk;
#ifdef STREAM_PREFETCHER
	if (!strcmp (cp->name, "ul2") && blk->spTag && !dl2_prefetch_active)
	{
		launch_sp(blk->spTag-1);
	}
	blk->spTag = 0;
#endif

	/* tag is unchanged, so hash links (if they exist) are still valid */

	/* record the last block to hit */
	cp->last_tagsetid.tag = CACHE_TAGSET (cp, addr);
	cp->last_tagsetid.threadid = threadid;
	cp->last_blk = blk;

	/* get user block data, if requested and it exists */
	if (udata)
		*udata = blk->user_data;

	/* return first cycle data is available to access */
	return (int) max2 (cp->hit_latency, (blk->ready - now));

cache_fast_hit:		/* fast hit handler */

#ifdef HOTLEAKAGE
	/* Leakage: for hits in low leak mode */

	if (blk->status & CACHE_BLK_DECAYED)
		fatal ("can't have decayed block in fast_hit");

	if (b_decay_profile_enabled)
		update_cache_block_stats_when_hit (cp, blk, cmd);
#endif //HOTLEAKAGE

	/* **FAST HIT** */
	if (!strcmp (cp->name, "dl0"))
		l0HitFlag = 1;
	if (!strcmp (cp->name, "dl1"))
		l1HitFlag = 1;
	if (!strcmp (cp->name, "il1") || !strcmp (cp->name, "il0"))
		il1HitFlag = 1;
	if (!strcmp (cp->name, "ul2"))
		l2HitFlag = 1;
	if (!strcmp (cp->name, "dtlb"))
		tlbHitFlag = 1;

	cp->hits[threadid]++;
#ifdef CACHE_THRD_STAT
	if (!strcmp (cp->name, "dl1"))
	{
		n_cache_access_thrd[threadid]++;
	}
#endif

	/* copy data out of cache block, if block exists */
	if (cp->balloc)
	{
		CACHE_BCOPY (cmd, blk, bofs, p, nbytes);
	}

#ifdef CFJ
	if (!strcmp (cp->name, "dl0") && cmd == Write && threadid == mainLeadThread)
		sbd_line_write(cp, addr, vp, nbytes, sbdLeadThread);
#endif

	/* update dirty status */
	if (cmd == Write)
	{
		blk->status |= CACHE_BLK_DIRTY;
		add_mask(blk->isByteDirty, CACHE_BLK (cp, addr), nbytes, cp->bsize);
#ifdef HOTLEAKAGE
		blk->time_dirty = sim_cycle[fastest_core];
#endif //HOTLEAKAGE
	}

	if (cmd == Write)
	{
		blk->last_access_time = now;
		blk->last_access_insn = new_sim_num_insn;
		blk->store_not_executed = new_store_not_exec;
		blk->wrongLoadCauses = LSQ[m_nLSQIdx]->recCauses;
		blk->poison = 0;
#ifdef CACHE_CLEANUP
		blk->robId = LSQ[m_nLSQIdx]->robId;
		blk->robExId = LSQ[m_nLSQIdx]->robExId;
#endif
	}
	if(cmd == Read && !strcmp (cp->name, "dl0") && blk->wrongLoadCauses && !LSQ[m_nLSQIdx]->isWrongPath && !LSQ[m_nLSQIdx]->spec_mode)
	{
		totalRecStoreLoadComm++;
		totalInstDelayStoreLoadComm += new_sim_num_insn - blk->last_access_insn;
	}
	blk->isStale = 0;
	prev_sim_cycle = blk->last_access_time;
	prev_sim_num_insn = blk->last_access_insn;
	prev_store_not_exec = blk->store_not_executed;
	current_blk = blk;

	/* Update access stats - Ronz */
	if (!strcmp (cp->name, "dl1"))
	{
		if (cmd == Write)
		{
			wh1++;
		}
		else
		{
			rh1++;
		}
	}
	if (!strcmp (cp->name, "ul2"))
	{
		if (cmd == Write)
		{
			wh2++;
		}
		else
		{
			rh2++;
		}
	}

	/* this block hit last, no change in the way list */

	/* tag is unchanged, so hash links (if they exist) are still valid */

	/* get user block data, if requested and it exists */
	if (udata)
		*udata = blk->user_data;

	/* record the last block to hit */
	cp->last_tagsetid.tag = CACHE_TAGSET (cp, addr);
	cp->last_tagsetid.threadid = threadid;
	cp->last_blk = blk;

	/* return first cycle data is available to access */
	//  if (!strcmp(cp->name,"dl1"))
	//  fprintf(stderr,"H %d\t%ld\t%d\n",set,now, max2(cp->hit_latency, (blk->ready - now)));
	return (int) max2 (cp->hit_latency, (blk->ready - now));
}				/* cache_access */

#ifdef CFJ
void sbd_line_write (struct cache_t *cp,	/* cache to access */
		md_addr_t addr,	/* address of access */
		void *vp,		/* ptr to buffer for input/output */
		int nbytes,	/* number of bytes to access */
		int threadid)
{
	byte_t *p = vp;
	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *blk;

	if (cp->hsize)
	{
		/* higly-associativity cache, access through the per-set hash tables */
		int hindex = CACHE_HASH (cp, tag);

		for (blk = cp->sets[set].hash[hindex]; blk; blk = blk->hash_next)
		{
			if ((blk->tagid.tag == tag) && (blk->tagid.threadid == threadid) &&
					(blk->status & CACHE_BLK_VALID))
				break;
		}
	}
	else
	{
		/* low-associativity cache, linear search the way list */
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) && (blk->tagid.threadid == threadid) &&
					(blk->status & CACHE_BLK_VALID))
				break;
		}
	}

	if (blk && cp->balloc && !is_mask_set(blk, vp, blk->isByteDirty, bofs, nbytes, cp->bsize))
	{
		blk->status |= CACHE_BLK_DIRTY;
		CACHE_BCOPY (Write, blk, bofs, p, nbytes);
	}
}
#endif

	void
cache_print (struct cache_t *cp1, struct cache_t *cp2)
{				/* Print cache access stats */
	fprintf (stderr, "ZZ L1D stats\n");
	fprintf (stderr, "ZZ %ld %ld %ld %ld %lu\n", rm1, rh1, wm1, wh1, (unsigned long) cp1->writebacks[0]);
	fprintf (stderr, "ZZ UL2 stats\n");
	if (cp2)
		fprintf (stderr, "ZZ %ld %ld %ld %ld %lu\n", rm2, rh2, wm2, wh2, (unsigned long) cp2->writebacks[0]);
}


	int
simple_cache_flush (struct cache_t *cp, int threadid)
{
	int i, numflush;
	struct cache_blk_t *blk;

	numflush = 0;
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
#ifdef CFJ
			if (blk->tagid.threadid == threadid && (blk->status & CACHE_BLK_VALID))
#else
			if (blk->status & CACHE_BLK_VALID)
#endif
			{
				cp->invalidations[blk->tagid.threadid]++;
				blk->status &= ~CACHE_BLK_VALID;
#ifdef CFJ
				blk->dup = 0;
#endif
				blk->last_access_time = 0;
				blk->last_access_insn = 0;
				blk->store_not_executed = 0;
				blk->wrongLoadCauses = 0;
				blk->isStale = 0;
				blk->poison = 0;
				clear_mask(blk->isByteDirty, cp->bsize);
#ifdef CACHE_CLEANUP
				blk->robId = 0;
				blk->robExId = 3;
#endif
#ifdef FINE_GRAIN_INV
				if(blk->wordStatus)
				{
					int k;
					for(k = 0; k < cp->word_num; k++)
						blk->wordStatus[k] = 0;
				}
#endif

				if (blk->status & CACHE_BLK_DIRTY)
				{
					cp->writebacks[blk->tagid.threadid]++;
					blk->status &= ~CACHE_BLK_DIRTY;
					numflush++;
				}
			}
		}
	}

	return numflush;
}

/* flush the entire cache, returns latency of the operation */
	unsigned int			/* latency of the flush operation */
cache_flush (struct cache_t *cp,	/* cache instance to flush */
		tick_t now)	/* time of cache flush */
{
	/* THIS HAS NOT BEEN CHANGED TO ACCOMMODATE MULTIPLE CACHE BANKS WITH
	   DIFFERENT ADDRESS RANGES !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

	int i, lat = cp->hit_latency;	/* min latency to probe cache */
	struct cache_blk_t *blk;

	/* blow away the last block to hit */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;

	/* no way list updates required because all blocks are being invalidated */
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
			if (blk->status & CACHE_BLK_VALID)
			{
				cp->invalidations[blk->tagid.threadid]++;
				blk->status &= ~CACHE_BLK_VALID;
				blk->last_access_time = 0;
				blk->last_access_insn = 0;
				blk->store_not_executed = 0;
				blk->wrongLoadCauses = 0;
				blk->poison = 0;
				blk->isStale = 0;
				clear_mask(blk->isByteDirty, cp->bsize);
#ifdef CACHE_CLEANUP
				blk->robId = 0;
				blk->robExId = 3;
#endif

				if (blk->status & CACHE_BLK_DIRTY)
				{
					/* write back the invalidated block */
					cp->writebacks[blk->tagid.threadid]++;
					cp->wrb++;
					if (!strcmp (cp->name, "dl1"))
						panic ("flush implementation should be modified for THRD_WAY_CACHE");
					lat += cp->blk_access_fn (Write, CACHE_MK_BADDR (cp, blk->tagid.tag, i), cp->bsize, blk, now + lat, blk->tagid.threadid);
				}
			}
		}
	}

	/* return latency of the flush operation */
	return lat;
}				/* cache_flush */

/* flush the block containing ADDR from the cache CP, returns the latency of
   the block flush operation */
	unsigned int			/* latency of flush operation */
cache_flush_addr (struct cache_t *cp,	/* cache instance to flush */
		md_addr_t addr,	/* address of block to flush */
		tick_t now,	/* time of cache flush */
		int threadid)
{
	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);
	struct cache_blk_t *blk;
	int lat = cp->hit_latency;	/* min latency to probe cache */

	if (cp->hsize)
	{
		/* higly-associativity cache, access through the per-set hash tables */
		int hindex = CACHE_HASH (cp, tag);

		for (blk = cp->sets[set].hash[hindex]; blk; blk = blk->hash_next)
		{
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */  &&
					(blk->status & CACHE_BLK_VALID))
				break;
		}
	}
	else
	{
		/* low-associativity cache, linear search the way list */
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */  &&
					(blk->status & CACHE_BLK_VALID))
				break;
		}
	}

	if (blk)
	{
		cp->invalidations[blk->tagid.threadid]++;
		blk->status &= ~CACHE_BLK_VALID;
		blk->last_access_time = 0;
		blk->isStale = 0;
		blk->last_access_insn = 0;
		blk->store_not_executed = 0;
		blk->wrongLoadCauses = 0;
		blk->poison = 0;
		clear_mask(blk->isByteDirty, cp->bsize);
#ifdef CACHE_CLEANUP
		blk->robId = 0;
		blk->robExId = 3;
#endif
#ifdef FINE_GRAIN_INV
		if(blk->wordStatus)
		{
			int k;
			for(k = 0; k < cp->word_num; k++)
				blk->wordStatus[k] = 0;

		}
#endif

		/* blow away the last block to hit */
		cp->last_tagsetid.tag = 0;
		cp->last_tagsetid.threadid = -1;
		cp->last_blk = NULL;

		if (blk->status & CACHE_BLK_DIRTY)
		{
			if (strcmp (cp->name, "dl0"))
			{
				/* write back the invalidated block */
				cp->writebacks[blk->tagid.threadid]++;
				if (!strcmp (cp->name, "dl1"))
					panic ("flush_addr implementation should be modified for THRD_WAY_CACHE");
				lat += cp->blk_access_fn (Write, CACHE_MK_BADDR (cp, blk->tagid.tag, set), cp->bsize, blk, now + lat, blk->tagid.threadid);
			}
			blk->status &= ~CACHE_BLK_DIRTY;
		}
		/* move this block to tail of the way (LRU) list */
		update_way_list (&cp->sets[set], blk, Tail, cp->assoc);
	}

	/* return latency of the operation */
	return lat;
}				/* cache_flush_addr */

	void
simple_cache_flush_addr (struct cache_t *cp,	/* cache instance to flush */
		md_addr_t addr)	/* address of block to flush */
{
	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);
	struct cache_blk_t *blk;

	if (cp->hsize)
	{
		/* higly-associativity cache, access through the per-set hash tables */
		int hindex = CACHE_HASH (cp, tag);

		for (blk = cp->sets[set].hash[hindex]; blk; blk = blk->hash_next)
		{
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */  &&
					(blk->status & CACHE_BLK_VALID))
				break;
		}
	}
	else
	{
		/* low-associativity cache, linear search the way list */
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) /*&& (blk->tagid.threadid == threadid) */  &&
					(blk->status & CACHE_BLK_VALID))
				break;
		}
	}


	if (blk 
#ifdef	CONV_PREF_STORES
			&& blk->isStale
#endif
	   )
	{
#ifdef	CONV_PREF_STORES
		totalRelCE++;
		if (blk->store_not_executed > blk->last_access_insn)	/* This line does not have the most updated data */
			totalRelCEStaleD++;
		else
		{
			if (lastTh0CommittedInsn < blk->last_access_insn)
				totalRelCECorrDBd++;
		}
#endif
		int comb_status = 0;
#ifdef FINE_GRAIN_INV
		if(blk->wordStatus)
		{
			int word_blk = CACHE_WORD_BLK(cp, addr);
			blk->wordStatus[word_blk] = 0;
			int k = 0;
			for(k = 0; k < cp->word_num; k++)
				comb_status = comb_status | blk->wordStatus[k];
		}
#endif	
		if(comb_status == 0)
		{
			blk->status &= ~CACHE_BLK_VALID;
			blk->last_access_time = 0;
			blk->isStale = 0;
			blk->last_access_insn = 0;
			blk->store_not_executed = 0;
			blk->wrongLoadCauses = 0;
			blk->poison = 0;
			clear_mask(blk->isByteDirty, cp->bsize);
#ifdef CACHE_CLEANUP
			blk->robId = 0;
			blk->robExId = 3;
#endif
			if (blk->status & CACHE_BLK_DIRTY)
				blk->status &= ~CACHE_BLK_DIRTY;

			/* blow away the last block to hit */
			cp->last_tagsetid.tag = 0;
			cp->last_tagsetid.threadid = -1;
			cp->last_blk = NULL;

			/* move this block to tail of the way (LRU) list */
			update_way_list (&cp->sets[set], blk, Tail, cp->assoc);
		}
	}
}

	void
cache_flush_page (md_addr_t addr,	/* Starting address */
		int nbytes)	/* Number of bytes to be flushed */
{
	struct cache_t *cp;
	struct cache_t *cache_list[5];
	int i;

	/* Would flush all the data caches for the given address range */
	cache_list[0] = cache_dl2;
	cache_list[1] = cache_dl1;
	cache_list[2] = cache_dl0;
	cache_list[3] = NULL;

	for (cp = cache_list[0], i = 0; cp != NULL; cp = cache_list[i])
	{
		md_addr_t memAddr = addr;

		memAddr = memAddr & (~cp->blk_mask);
		while (memAddr < (addr + nbytes))
		{
			simple_cache_flush_addr (cp, memAddr);
			memAddr = memAddr + cp->bsize;
		}
		i++;
	}
}

#ifdef CFJ
counter_t mainDirtySbdDirty = 0;
counter_t mainDirtySbdClean = 0;
counter_t mainClean = 0;

void cache_reset_id (struct cache_t *cp, int threadid)
{
	int i, lat = cp->hit_latency;	/* min latency to probe cache */
	struct cache_blk_t *blk;

	/* blow away the last block to hit */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;

	/* no way list updates required because all blocks are being invalidated */
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
			if (blk->status & CACHE_BLK_VALID)
			{
				struct cache_blk_t *blk1, *blk_flush;
				int match = 0;
				for (blk1 = cp->sets[i].way_head; blk1; blk1 = blk1->way_next)
				{
					if (blk1->status & CACHE_BLK_VALID)
					{
						if(blk1->tagid.tag == blk->tagid.tag && blk1->tagid.threadid == blk->tagid.threadid)
							continue;
						if(blk1->tagid.tag == blk->tagid.tag)
						{
							if(match)
								panic("More than one match in a single way of L0 cache.");
							match = 1;
							if(blk1->tagid.threadid != threadid)
							{
								blk_flush = blk1;
								blk1 = blk;
							}
							else
								blk_flush = blk;

							if((blk_flush->status & CACHE_BLK_DIRTY) && (blk1->status & CACHE_BLK_DIRTY))
								mainDirtySbdDirty++;
							else if((blk_flush->status & CACHE_BLK_DIRTY) && !(blk1->status & CACHE_BLK_DIRTY)) 
							{
								memcpy(blk1->data, blk_flush->data, cp->bsize);
                                mainDirtySbdClean++;
							}
							else
								mainClean++;

							blk_flush->status = 0;
							blk_flush->last_access_time = 0;
							blk_flush->isStale = 0;
							blk_flush->last_access_insn = 0;
							blk_flush->store_not_executed = 0;
							blk_flush->wrongLoadCauses = 0;
							blk_flush->poison = 0;
#ifdef CACHE_CLEANUP
							blk_flush->robId = 0;
							blk_flush->robExId = 3;
#endif
							cp->last_tagsetid.tag = 0;
							cp->last_tagsetid.threadid = -1;
							cp->last_blk = NULL;
							clear_mask(blk_flush->isByteDirty, cp->bsize);
							update_way_list (&cp->sets[i], blk_flush, Tail, cp->assoc);
						}
					}
				}
				blk->tagid.threadid = threadid;
				blk->dup = 0;
			}
		}
	}
}

void cache_reset_dirty (struct cache_t *cp)
{
	int i, lat = cp->hit_latency;	/* min latency to probe cache */
	struct cache_blk_t *blk;

	/* no way list updates required because all blocks are being invalidated */
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
			if ((blk->status & CACHE_BLK_VALID) && (blk->status & CACHE_BLK_DIRTY))
			{
				blk->status &= ~CACHE_BLK_DIRTY;
				clear_mask(blk->isByteDirty, cp->bsize);
			}
		}
	}
}
#endif

#ifdef HOTLEAKAGE
/* Leakage: extra routines added here */

	double
ln2 (double x)
{
	return log10 (x) / log10 (2.0);
}

	void
cache_leak_init (void)
{

	n_lines = decayed_cache->nsets * decayed_cache->assoc;
	cache_size = n_lines * decayed_cache->bsize;

	/* local width =  bits_per_counter      */
	local_width = ln2 (local_counter_max + 1);

	/* global width = bits_per_counter      */
	global_width = ln2 (global_counter_max + 1);

	/* tag size = virtual_addr_size - ln(nsets*bsize)       */
	v_addr_size = sizeof (md_addr_t) * 8;
	tag_size = v_addr_size - (ln2 (decayed_cache->nsets) + ln2 (decayed_cache->bsize)) + CACHE_BLK_NFLAGS;
	tag_array_size = (tag_size * n_lines) / 8;	/* bytes */

	/* store the power estimates for leakage modeling overhead      */

	/* overhead power modeled as simple array  */
	global_af = 1.0 / global_width;	/* assuming gray coding */
	local_af = 0.5;

	/* global counter power - per access    */
	global_access_power = simple_array_wordline_power (1, global_width, 1, 1, 0);
	/* no decoder power - may need to FIX THIS */
	global_access_power += global_af * simple_array_bitline_power (1, global_width, 1, 1, 0);

	/* local counter power - per access     */
	local_access_power = simple_array_wordline_power (n_lines, local_width, 1, 1, 0);
	/* no decoder power - may need to FIX THIS */
	local_access_power += local_af * simple_array_bitline_power (n_lines, local_width, 1, 1, 0);

	/* power per local counter reset */
	local_reset_power = simple_array_wordline_power (1, local_width, 1, 1, 0);
	/* no decoder power - may need to FIX THIS */
	local_reset_power += local_af * simple_array_bitline_power (1, local_width, 1, 1, 0);
}

/*
 * ratio between 'xtra' leakage and 'orig' leakage.
 * note that we are approximating the counters and
 * associated logic as array  structures. if not,
 * appropriate leakage calculation routine in to be
 * called in this function
 */

	double
cache_get_ctrl_ovhd_ratio (enum leak_ctrl_type_t type)
{
	double xtra2orig_ratio;

	switch (type)
	{
		/* assuming all of these use n bit counters     */
		case GatedVss:
		case Drowsy:
		case RBB:
			/* n bits for every line and a constant global counter  */
			xtra2orig_ratio = ((double) local_width) / (decayed_cache->bsize * 8 + tag_size);
			xtra2orig_ratio += ((double) global_width) / (8 * (cache_size + tag_array_size));
			break;
		case None:
		default:
			xtra2orig_ratio = 0;
			break;
	}

	return xtra2orig_ratio;
}

/*
 * get an estimate of dynamic power for the overhead logic per cycle
 * Note: right now, the counter array of the xtra logic is considered
 * as a simple array with n_cache_lines rows and bits_per_counter
 * columns.
 */

	double
cache_get_xtra_power (enum leak_ctrl_type_t type)
{
	double xtra_power = 0;

	switch (type)
	{
		/* assuming all of these use n bit counters     */
		case GatedVss:
		case Drowsy:
		case RBB:

			/* global counter ticks every cycle -  so charge one global access every cycle */
			xtra_power += global_access_power;

			/* power due to any counter resets */
			xtra_power += n_resets * local_reset_power;

			/* power due to update of local counters */
			if (global_tick)
				xtra_power += local_access_power;

			break;

		case None:
		default:
			break;
	}

	/* reset measurements   */
	global_tick = 0;
	n_resets = 0;

	return xtra_power;
}

#endif //HOTLEAKAGE

/* Copy nbytes of data from/to cache block */
	void
cacheBcopy (enum mem_cmd cmd, struct cache_blk_t *blk, int position, byte_t * data, int nbytes)
{
	CACHE_BCOPY (cmd, blk, position, data, nbytes);
}

/* Search cache for address block */
	int
cacheBlockSearch (struct cache_t *cp, enum mem_cmd cmd, md_addr_t addr, byte_t * data, int nbytes, int threadid)
{
	md_addr_t tag;
	md_addr_t set;
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *blk = NULL;
	md_addr_t bank_per_thread, thrd_bank_start, thrd_word_bank;
	md_addr_t set_bank;
	md_addr_t wb_addr;
	int hit = 0;

	tag = CACHE_TAG (cp, addr);
	set = CACHE_SET (cp, addr);


#if 0
	if (addr == 0x140002bf0 && !strcmp (cp->name, "dl0"))
	{
		fprintf (stderr, "Debug here\n");
	}
#endif

	/* low-associativity cache, linear search the way list */
	for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
	{
		/* ThreadID chech is not required for slipstream processor */
		if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID) 
#ifdef CFJ
			&& (strcmp(cp->name,"dl0") || (!thecontexts[sbdLeadThread]->running || blk->tagid.threadid == threadid || (threadid == sbdLeadThread && !blk->dup) || (threadid == mainLeadThread && !(blk->status & CACHE_BLK_DIRTY))))
#endif
			)
		{
			hit = 1;
			current_blk = blk;
			break;
		}
	}

#ifdef FINE_GRAIN_INV
	if(hit && blk->wordStatus)
	{
		int word_blk = CACHE_WORD_BLK(cp, addr);
		if(blk->wordStatus[word_blk] == 0) hit = 0;
	}
#endif
	if ((hit == 1) && (cp->balloc))
	{
#ifdef  IDEAL_LONGDIST_CE
		extern counter_t totalRelCE;
		extern int currentGear;

		/* Evict the cache if L0 contains cache line older then 5000 instructions,
		 ** because we must get the good data from L1. */
		if (currentGear == 1 && (blk->store_not_executed > blk->last_access_insn) && (new_sim_num_insn - blk->store_not_executed) > 100)
		{
			totalRelCE++;
			blk->status &= ~CACHE_BLK_VALID;
			blk->last_access_time = 0;
			blk->isStale = 0;
			blk->last_access_insn = 0;
			blk->store_not_executed = 0;
			blk->wrongLoadCauses = 0;
			blk->poison = 0;
#ifdef CACHE_CLEANUP
			blk->robId = 0;
			blk->robExId = 3;
#endif
			if (blk->status & CACHE_BLK_DIRTY)
				blk->status &= ~CACHE_BLK_DIRTY;

			/* blow away the last block to hit */
			cp->last_tagsetid.tag = 0;
			cp->last_tagsetid.threadid = -1;
			cp->last_blk = NULL;

			/* move this block to tail of the way (LRU) list */
			update_way_list (&cp->sets[set], blk, Tail, cp->assoc);
			return 0;
		}
		else
		{
#endif
			CACHE_BCOPY (cmd, blk, bofs, data, nbytes);
			return 1;
#ifdef  IDEAL_LONGDIST_CE
		}
#endif
	}
	return 0;
}

	void
cacheAllocBlock (struct cache_t *cp, md_addr_t addr, byte_t * data, int nbytes)
{
	md_addr_t tag;
	md_addr_t set;
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *repl = NULL;
	md_addr_t bank_per_thread, thrd_bank_start, thrd_word_bank;
	md_addr_t set_bank;
	int threadid = 0;

	tag = CACHE_TAG (cp, addr);
	set = CACHE_SET (cp, addr);

	/* select the appropriate block to replace, and re-link this entry to
	   the appropriate place in the way list */
	repl = cp->sets[set].way_tail;
	update_way_list (&cp->sets[set], repl, Head, cp->assoc);

	/* remove this block from the hash bucket chain, if hash exists */
	if (cp->hsize)
		unlink_htab_ent (cp, &cp->sets[set], repl);

	/* blow away the last block to hit */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;
	/* update block tags */
	repl->tagid.tag = tag;
	repl->tagid.threadid = threadid;
	repl->status = CACHE_BLK_VALID;	/* dirty bit set on update */
	repl->ready = 0;

	CACHE_BCOPY (Write, repl, bofs, data, nbytes);

	/* link this entry back into the hash table */
	if (cp->hsize)
		link_htab_ent (cp, &cp->sets[set], repl);
}

	int
storeSearch (struct cache_t *cp, md_addr_t addr, counter_t sim_num_insn)
{
	md_addr_t tag;
	md_addr_t set;
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *blk = NULL;
	md_addr_t bank_per_thread, thrd_bank_start, thrd_word_bank;
	md_addr_t set_bank;
	md_addr_t wb_addr;
	int hit = 0;
	int threadid = 0;

	tag = CACHE_TAG (cp, addr);
	set = CACHE_SET (cp, addr);

	/* low-associativity cache, linear search the way list */
	for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
	{
		/* ThreadID chech is not required for slipstream processor */
		if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
		{
			hit = 1;
			break;
		}
	}

	if (hit == 1)
	{
		blk->store_not_executed = sim_num_insn;
		return 1;
	}
	return 0;
}

	int
cachePollution (struct cache_t *cp)
{
	md_addr_t set;
	struct cache_blk_t *blk = NULL;
	int numPolluted = 0;

	/* low-associativity cache, linear search the way list */
	for (set = 0; set < cp->nsets; set++)
	{
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			/* ThreadID chech is not required for slipstream processor */
			if (blk->status & CACHE_BLK_VALID && (blk->poison || blk->wrongLoadCauses))
				numPolluted++;
		}
	}
	return numPolluted;
}

#ifdef STREAM_PREFETCHER
	void
init_sp ()
{
	int i;

	for (i = 0; i < MISS_TABLE_SIZE; i++)
		miss_history_table.history[i] = 0;
	miss_history_table.mht_tail = 0;
	miss_history_table.mht_num = 0;

	for (i = 0; i < STREAM_ENTRIES; i++)
	{
		stream_table[i].valid = 0;
		stream_table[i].addr = 0;
		stream_table[i].stride = 0;
		stream_table[i].remaining_prefetches = 0;
		stream_table[i].last_use = 0;
	}
}

	void
insert_sp (md_addr_t addr)
{				/* This function is called if L2 cache miss is discovered. */
	int i, num;
	int dist;
	int min_entry;
	md_addr_t stride_addr;
	int count = 0;

	if (miss_history_table.mht_num == 0)
	{				/* Table is empty */
		miss_history_table.history[miss_history_table.mht_tail] = (addr & ~(cache_dl2->bsize - 1));
		miss_history_table.mht_tail = (miss_history_table.mht_tail + 1) % MISS_TABLE_SIZE;
		miss_history_table.mht_num++;
		return;
	}

	dist = (addr & ~(cache_dl2->bsize - 1)) - miss_history_table.history[(miss_history_table.mht_tail + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE];
	stride_addr = miss_history_table.history[(miss_history_table.mht_tail + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE];
	min_entry = (miss_history_table.mht_tail + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE;

	/* First calculate the minimum DELTA */
	for (num = 1, i = (miss_history_table.mht_tail + MISS_TABLE_SIZE - 2) % MISS_TABLE_SIZE; num < miss_history_table.mht_num; i = (i + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE, num++)
	{
		int curr_dist = (addr & ~(cache_dl2->bsize - 1)) - miss_history_table.history[i];

		if (abs (curr_dist) < abs (dist))
		{
			dist = curr_dist;
			stride_addr = miss_history_table.history[i];
			min_entry = i;
		}
	}

	i = min_entry;
	do
	{
		i = (i + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE;
		int curr_dist = (stride_addr & ~(cache_dl2->bsize - 1)) - miss_history_table.history[i];

		if (curr_dist == dist)
		{
			count++;
			stride_addr = miss_history_table.history[i];
		}
	}
	while (i != miss_history_table.mht_tail);

	/* Now we can insert this miss entry into the miss history table */
	miss_history_table.history[miss_history_table.mht_tail] = (addr & ~(cache_dl2->bsize - 1));
	miss_history_table.mht_tail = (miss_history_table.mht_tail + 1) % MISS_TABLE_SIZE;
	if (miss_history_table.mht_num < MISS_TABLE_SIZE)
		miss_history_table.mht_num++;

	if (count > 1)
	{				/* We hit a stride. Insert the stride into stream table. Also initiate the first prefetch. 
					   If the table is completely full, use LRU algorithm to evict one entry. */
		int id = 0;

		if (stream_table[0].valid == 1)
		{
			int last_use = stream_table[0].last_use;

			id = 0;
			for (i = 1; i < STREAM_ENTRIES; i++)
			{
				if (stream_table[i].valid == 1)
				{
					if (last_use < stream_table[i].last_use)
					{
						last_use = stream_table[i].last_use;
						id = i;
					}
				}
				else
				{
					id = i;
					break;
				}
			}
		}

		/* Insert the stride in this entry */
		stream_table[id].valid = 1;
		stream_table[id].addr = addr & ~(cache_dl2->bsize - 1);
		stream_table[id].stride = dist;
		stream_table[id].remaining_prefetches = PREFETCH_DISTANCE;
		stream_table[id].last_use = 2000000000;

		/* Now call a prefetch for this addr */
		launch_sp (id);
	}
}

	void
launch_sp (int id)
{				/* This function is called when access in L2 to pre-fetch data is discovered. */
	int i = id;

	if (stream_table[i].valid == 1)
	{
		for(; stream_table[i].remaining_prefetches; stream_table[i].remaining_prefetches--)
		{
			int load_lat = 0;
			byte_t *l1cache_line = calloc (cache_dl2->bsize, sizeof (byte_t));

			stream_table[i].addr = stream_table[i].addr + stream_table[i].stride;
			stream_table[i].last_use = sim_cycle[fastest_core];

			/* Tell the system that this call at this time is a prefetch, so stream
			 ** prefetch must not trigger.*/
			dl2_prefetch_active = 1;
			dl2_prefetch_id = i;
			load_lat = cache_access (cache_dl2, Read, (stream_table[i].addr & ~(cache_dl2->bsize - 1)), l1cache_line, cache_dl2->bsize,
					/* now */ sim_cycle[fastest_core], /* pudata */ NULL, /* repl addr */ NULL, 0);
			if (l2HitFlag)
			{
				if (load_lat <= cache_dl2->hit_latency)
					pfl2Hit++;
				else
					pfl2SecMiss++;
			}
			else
				pfl2PrimMiss++;

			dl2_prefetch_active = 0;
			free (l1cache_line);
		}
	}
	stream_table[i].remaining_prefetches = 1;
}

#endif

#ifdef  REALISTIC_LONGDIST_CE
extern counter_t totalRelCE;
extern counter_t totalRelCEStaleD;
extern counter_t totalRelCECorrDBd;
	void
flushSelective (struct cache_t *cp)
{
	int i, lat = cp->hit_latency;	/* min latency to probe cache */
	struct cache_blk_t *blk;
	extern int lastTh0CommittedInsn;

	/* blow away the last block to hit */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;

	/* First try to invalidate all the cache lines that may be stale. 
	 ** If any valid cache line is touched during the last interval would not be fulshed.
	 ** Those cache lines which are flushed mark them as stale. */
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
			if (blk->status & CACHE_BLK_VALID)
			{
#ifndef STORE_DELAY_CE
				if (blk->isStale)
				{
					totalRelCE++;
					if (blk->store_not_executed > blk->last_access_insn)	/* This line does not have the most updated data */
						totalRelCEStaleD++;
					else
					{
						if (lastTh0CommittedInsn < blk->last_access_insn)
							totalRelCECorrDBd++;
					}
					blk->status = 0;
					blk->last_access_time = 0;
					blk->last_access_insn = 0;
					blk->store_not_executed = 0;
					blk->wrongLoadCauses = 0;
					blk->poison = 0;
					blk->isStale = 0;
#ifdef CACHE_CLEANUP
					blk->robId = 0;
					blk->robExId = 3;
#endif
				}
				else
#endif
					blk->isStale = 1;
			}
		}
	}
}
#endif
void markCacheOld(struct cache_t *cp, int robExId)
{
	struct cache_blk_t *blk;
	int i;

	for (i=0; i<cp->nsets; i++)
	{
		for (blk=cp->sets[i].way_head; blk; blk=blk->way_next)
		{
			if (blk->robExId == robExId)
				blk->robExId = 3;
		}
	}
}

#ifdef ACCURATE_CACHE_MISS_IMP
struct repl_live_t *repl_live = NULL;
void insert_repl_live(md_addr_t addr, tick_t ready, tick_t recycle, tick_t now, byte_t *data)
{
	if(recycle <= now) return;
	if(ready > recycle) return;
	byte_t *p = data;
	md_addr_t rofs = 0;

	struct repl_live_t *temp = (struct repl_live_t *) calloc(1, sizeof(struct repl_live_t));
	temp->addr = addr;
	temp->ready = ready;
	temp->recycle = recycle;
	temp->data = calloc(cache_dl2->bsize, sizeof(byte_t));
	CACHE_SCOPY(Write, temp->data, rofs, p, cache_dl2->bsize);
	temp->next = NULL;
	struct repl_live_t *traverse;
	struct repl_live_t *previous;

	if(repl_live == NULL)
	{
		repl_live = temp;
		return;
	}
	if(recycle < repl_live->recycle)
	{
		temp->next = repl_live;
		repl_live = temp;
		return;
	} 
	previous = repl_live;
	traverse = repl_live->next;
	while(traverse)
	{
		if(recycle < traverse->recycle)
		{
			temp->next = previous->next;
			previous->next = temp;
			return;
		}
		traverse = traverse->next;
		previous = previous->next;
	}
	previous->next = temp;
}

void remove_repl_live(counter_t now)
{
	while(repl_live && repl_live->recycle < now)
	{
		struct repl_live_t *temp = repl_live;
		repl_live = repl_live->next;
		free(temp->data);
		free(temp);
	}
}

struct repl_live_t * search_repl_live(md_addr_t addr)
{
	struct repl_live_t *traverse = repl_live;
	while(traverse)
	{
		if(traverse->addr == addr)
			return traverse;
		traverse = traverse->next;
	}
	return NULL;
}
#endif

unsigned long long int * create_byte_mask(int bsize)
{
	int num = 1, i;
	unsigned long long int * temp;
	if(bsize > (sizeof(unsigned long long int)*8))
		num += (bsize-1)/(sizeof(unsigned long long int)*8);

	temp = (unsigned long long int *) calloc(num, sizeof(unsigned long long int));
	for(i = 0; i < num; i++)
		temp[i] = 0;
	return temp;
}

void clear_mask(unsigned long long int *mask, int bsize)
{
	int num = 1, i;
	if(bsize > (sizeof(unsigned long long int)*8))
		num += (bsize-1)/(sizeof(unsigned long long int)*8);
	for(i = 0; i < num; i++)
		mask[i] = 0;
}

void add_mask(unsigned long long int *mask, int bofs, int nbytes, int bsize)
{
	int index, i;
	if(bofs >= bsize)
		panic("Wrong byte offset sent\n");

	index = bofs / (sizeof(unsigned long long int)*8);
	bofs = bofs % (sizeof(unsigned long long int)*8);
	for(i = 0; i < nbytes; i++)
	{
		mask[index] |= (1<<bofs);
		bofs++;
	}
}

counter_t byteDirty = 0;
int is_mask_set(struct cache_blk_t *blk, byte_t *vp, unsigned long long int *mask, int bofs, int nbytes, int bsize)
{
	int index, i;
	int bofs_new;
	if(bofs >= bsize)
		panic("Wrong byte offset sent\n");

	index = bofs / (sizeof(unsigned long long int)*8);
	bofs_new = bofs % (sizeof(unsigned long long int)*8);
	for(i = 0; i < nbytes; i++)
	{
		if(mask[index] & (1<<bofs_new))
		{
			byteDirty++;
			return 1;
			int byte = 1;
			int bof = bofs+i;
			byte_t *p = &vp[i];
			CACHE_BCOPY (Read, blk, bof, p, byte);
		}
		bofs_new++;
	}
	return 0;
}
