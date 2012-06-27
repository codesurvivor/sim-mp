/*
 * cache.c - cache module routines
 *
 * This file is a part of the SimpleScalar tool suite written by
 {
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
 *
 * Revision 1.1  1996/12/05  18:52:32  taustin
 * Initial revision
 *
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "headers.h"

int data_packet_size = 5;        
int meta_packet_size = 1;        

/* extract/reconstruct a block address */
#define CACHE_BADDR(cp, addr)	((addr) & ~(cp)->blk_mask)
#define CACHE_MK_BADDR(cp, tag, set)					\
	(((tag) << (cp)->tag_shift)|((set) << (cp)->set_shift))

#define CACHE_MK1_BADDR(cp, tag, set)					\
	((((tag) << (cp)->tag_shift)/BANKS)|((set) << (cp)->set_shift))

#define CACHE_MK2_BADDR(cp, tag, set)					\
	(((tag) << ((cp)->tag_shift - (log_base2(res_membank)-log_base2(n_cache_limit_thrd[threadid])) ))| \
	 ((set) << ((cp)->set_shift - (log_base2(res_membank)-log_base2(n_cache_limit_thrd[threadid])) )))


/* index an array of cache blocks, non-trivial due to variable length blocks */
#define CACHE_BINDEX(cp, blks, i)					\
	((struct cache_blk_t *)(((char *)(blks)) +				\
		(i)*(sizeof(struct cache_blk_t) +		\
			((cp)->balloc				\
			 ? (cp)->bsize*sizeof(byte_t) : 0))))

/* cache data block accessor, type parameterized */
#define __CACHE_ACCESS(type, data, bofs)				\
	(*((type *)(((char *)data) + (bofs))))

/* cache data block accessors, by type */
#define CACHE_DOUBLE(data, bofs)  __CACHE_ACCESS(double, data, bofs)
#define CACHE_FLOAT(data, bofs)	  __CACHE_ACCESS(float, data, bofs)
#define CACHE_WORD(data, bofs)	  __CACHE_ACCESS(unsigned int, data, bofs)
#define CACHE_HALF(data, bofs)	  __CACHE_ACCESS(unsigned short, data, bofs)
#define CACHE_BYTE(data, bofs)	  __CACHE_ACCESS(unsigned char, data, bofs)

/* cache block hashing macros, this macro is used to index into a cache
   set hash table (to find the correct block on N in an N-way cache), the
   cache set index function is CACHE_SET, defined above */
#define CACHE_HASH(cp, key)						\
	(((key >> 24) ^ (key >> 16) ^ (key >> 8) ^ key) & ((cp)->hsize-1))

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

/* bound squad_t/dfloat_t to positive int */
#define BOUND_POS(N)		((int)(min2(max2(0, (N)), 2147483647)))

/* Ronz' stats */

#define BANKS 16		/* Doesn't matter unless you use address-ranges. Same for bank_alloc. */
/*
   int bank_alloc[CACHEPORT] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
 */

/****************************************************/
#ifdef CROSSBAR_INTERCONNECT
int busSlotCount = BUSSLOTS * NUMBEROFBUSES;
extern long long int busUsed;
extern counter_t busOccupancy;
struct bs_nd *busNodePool;
struct bs_nd *crossbarSndHd[3][MAXTHREADS], *crossbarRcvHd[3][MAXTHREADS];
counter_t sendCount[3][MAXTHREADS], rcvCount[3][MAXTHREADS];
#endif
extern counter_t dcache2_access;

#ifdef BUS_INTERCONNECT
int busSlotCount = BUSSLOTS * NUMBEROFBUSES;
extern long long int busUsed;
extern counter_t busOccupancy;
struct bs_nd *busNodePool, *busNodesInUse[NUMBEROFBUSES];
#endif
/*also in sim-outorder.c*/
//Jing 060208
char dl1name[20];

extern int cache_dl1_lat;
extern int cache_il1_lat;
extern int cache_dl2_lat;
extern int mesh_size;
extern int mem_bus_width;
extern float mem_bus_speed;
extern int mem_port_num;
int max_packet_size = 32; //32byte
extern counter_t cacheMiss[MAXTHREADS], hitInOtherCache[MAXTHREADS];
counter_t involve_4_hops, involve_2_hops, involve_2_hop_wb, involve_2_hop_touch, involve_2_hops_wm, involve_4_hops_wm, involve_4_hops_upgrade, involve_2_hops_upgrade;
counter_t involve_4_hops_miss, involve_2_hops_miss;
counter_t data_private_read, data_private_write, data_shared_read, data_shared_write;
extern counter_t load_link_shared, load_link_exclusive;
extern counter_t spand[5];
extern counter_t downgrade_w, downgrade_r;

counter_t total_all_close, total_all_almostclose, total_not_all_close, total_p_c_events, total_consumers, total_packets_in_neighbor, total_packets_at_corners;
counter_t total_data_at_corner, total_data_close, total_data_far, total_data_consumers;
counter_t sharer_num[MAXTHREADS];
double average_inside_percent;
double average_outside_percent;
double average_outside_abs_percent;
double average_corner_percent;
counter_t write_early;
extern long long int dl2ActuallyAccessed, markedLineReplaced;
extern counter_t totalSplitWM, totalSplitNo;
extern counter_t totalUpgradesUsable, totalUpgradesBeneficial;

extern int actualProcess;

int dataFoundInPCB (md_addr_t, int, int);
void invalidateOtherPCB (int who, int id, md_addr_t addr);

/****************************************************/
int dl2_prefetch_active = 0;
int dl2_prefetch_id = 0;
int dl1_prefetch_active = 0;


extern struct quiesceStruct quiesceAddrStruct[CLUSTERS];
extern spec_benchmarks;
extern struct res_pool *fu_pool;
extern struct cache_t *cache_dl1[], *cache_dl2;
//#define LOCK_CACHE_REGISTER
//#define BAR_OPT
#ifdef LOCK_CACHE_REGISTER
struct LOCK_CACHE {
	md_addr_t tag; /* lock address which is now subscribed */
	unsigned long long int SubScribeVect; /* subscribe bector */
	int lock_owner; /* acquiring entry */
	int BoolValue; /* value of the lock */
} Lock_cache[MAXTHREADS];
#define MAXLOCKEVENT  8
struct DIRECTORY_EVENT *lock_fifo[MAXTHREADS][MAXLOCKEVENT];
int lock_fifo_num[MAXTHREADS];
int lock_fifo_head[MAXTHREADS];
int lock_fifo_tail[MAXTHREADS];
extern counter_t lock_fifo_full;
extern counter_t lock_fifo_wrong;
extern counter_t lock_fifo_writeback;
extern counter_t lock_fifo_benefit;
extern counter_t lock_cache_hit;
extern counter_t lock_cache_miss;
#endif
extern md_addr_t barrier_addr;

extern int markReadAfterWrite;

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

#ifdef   THRD_WAY_CACHE
extern int activecontexts;
extern int res_membank;
extern int n_cache_limit_thrd[];
extern int n_cache_start_thrd[];
extern int COHERENT_CACHE;
extern int MSI;
#endif

extern context *thecontexts[MAXTHREADS];

extern FILE *outFile;
extern int collect_stats, allForked;

#ifdef   THRD_WAY_CACHE
void insert_fillq (long, md_addr_t, int);
#else
void insert_fillq (long, md_addr_t);	/* When a cache discovers a miss, */
#endif

extern int numcontexts;
extern int notRoundRobin;

extern struct RS_list *rs_cache_list[MAXTHREADS][MSHR_SIZE];
extern FILE *fp_trace;
extern struct DIRECTORY_EVENT *dir_event_queue;  /* Head pointer point to the directory event queue */
extern struct DIRECTORY_EVENT *event_list[L2_MSHR_SIZE];  /* event list for L2 MSHR */
counter_t nack_counter; 
counter_t normal_nacks;
counter_t write_nacks;
counter_t prefetch_nacks;
counter_t sync_nacks;
counter_t flip_counter;
counter_t store_conditional_failed;
counter_t L1_flip_counter;
counter_t e_to_m;
/* queue counter */
extern counter_t L1_mshr_full;
extern counter_t last_L1_mshr_full[MAXTHREADS];
extern counter_t L2_mshr_full;
extern counter_t last_L2_mshr_full;
extern counter_t L2_mshr_full_prefetch;
extern counter_t L1_fifo_full;
extern counter_t last_L1_fifo_full[MAXTHREADS+MESH_SIZE*2];
extern counter_t Dir_fifo_full;
extern counter_t last_Dir_fifo_full[MAXTHREADS+MESH_SIZE*2];
extern counter_t Input_queue_full;
extern counter_t last_Input_queue_full[MAXTHREADS+MESH_SIZE*2];
extern counter_t Output_queue_full;

extern counter_t Stall_L1_mshr;
extern counter_t Stall_L2_mshr;
extern counter_t Stall_L1_fifo;
extern counter_t Stall_dir_fifo;
extern counter_t Stall_input_queue;
extern counter_t Stall_output_queue;
extern int mshr_pending_event[MAXTHREADS];

extern counter_t WM_Miss;
extern counter_t WM_Clean;
extern counter_t WM_S;
extern counter_t WM_EM;
extern counter_t write_shared_used_conf;

extern counter_t SyncInstCacheAccess;
extern counter_t TestCacheAccess;
extern counter_t TestSecCacheAccess;
extern counter_t SetCacheAccess;
extern counter_t SyncLoadReadMiss;
extern counter_t SyncLoadLReadMiss;
extern counter_t SyncLoadHit;
extern counter_t SyncLoadLHit;
extern counter_t SyncStoreCHit;
extern counter_t SyncStoreCWriteMiss;
extern counter_t SyncStoreCWriteUpgrade;
extern counter_t SyncStoreHit;
extern counter_t SyncStoreWriteMiss;
extern counter_t BarStoreWriteMiss;
extern counter_t SyncStoreWriteUpgrade;
extern counter_t BarStoreWriteUpgrade;
extern counter_t Sync_L2_miss;


#ifdef CONF_RES_RESEND
extern struct QUEUE_EVENT *send_queue[MAXTHREADS];
extern struct QUEUE_EVENT *reply_queue[MAXTHREADS];
#endif

extern struct RS_link *event_queue;   
extern long long int totalEventCount, totalEventProcessTime;
long long int totaleventcount;
counter_t popnetMsgNo;
counter_t missNo;
counter_t write_back_msg;
counter_t local_cache_access, remote_cache_access, localdirectory, remotedirectory;
tick_t totalmisshandletime;
tick_t totalIL1misshandletime;
long long int totaleventcountnum;
counter_t totalWriteIndicate;
counter_t total_exclusive_modified;
counter_t total_exclusive_conf;
counter_t total_exclusive_cross;
counter_t totalSyncEvent;
counter_t totalNormalEvent;
counter_t totalSyncWriteM;
counter_t totalSyncReadM;
counter_t totalSyncWriteup;
counter_t totalmisstimeforNormal;
counter_t totalmisstimeforSync;
counter_t total_mem_lat;
counter_t total_mem_access;
counter_t totalL2misstime;
counter_t totalWrongL2misstime;
counter_t TotalL2misses;
long long int total_L1_prefetch;
counter_t l2_prefetch_num;
counter_t write_back_early;
long long int total_L1_first_prefetch;
long long int total_L1_sec_prefetch;
counter_t L1_prefetch_usefull;
counter_t L1_prefetch_writeafter;
#ifdef PREFETCH_OPTI
counter_t ReadPrefetchUpdate;		//Read prefetch coherence update
counter_t WritePrefetchUpdate;		//Write prefetch coherence update
counter_t ReadMetaReply_CS;			//Meta reply for read prefetch update (directory in clean or shared state)
counter_t ReadDataReply_CS;			//Data reply for read prefetch update (directory in clean or shared state, but some one modified this data before, see prefetch_invalidate and prefetch_modified)
counter_t ReadDowngrade_EM;			//Downgrade request is sending out if the directory is in exclusive/modified state, take prefetch update message as a read miss
counter_t ReadL2Miss;				//Read prefetch update is a L2 miss
counter_t ReadEMetaReply;
counter_t ReadEDataReply;
counter_t ReadMReply;
counter_t WriteMetaReply_C;			//Meta reply for write prefetch update (directory in clean state)	
counter_t WriteDataReply_C;			//Data reply for write prefetch update (directory in clean state, but someone modified this date before, see prefetch_invalidate and prefetch_modified)
counter_t WriteInvalidate_SEM;		//Invalidation request is sending out if the directory is in exclusive/modified state, take prefetch update message as a write update
counter_t WriteL2Miss;				//Write prefetch update is a L2 miss
counter_t WriteSEMetaReply;
counter_t WriteSEDataReply;
counter_t WriteMReply;
#endif
long long int totalreqcountnum;
long long int totalmisscountnum;
int flag;

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

/* where to insert a block onto the ordered way chain */
enum list_loc_t
{ Head, Tail };

/* insert BLK into the order way chain in SET at location WHERE */
	static void
update_way_list (struct cache_set_t *set,	/* set contained way chain */
		struct cache_blk_t *blk,	/* block to insert */
		enum list_loc_t where)	/* insert location */
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
	else
		panic ("bogus WHERE designator");
}

/* create and initialize a general cache structure */
struct cache_t *		/* pointer to cache created */
cache_create (char *name,	/* name of the cache */
		int nsets,	/* total number of sets in cache */
		int bsize,	/* block (line) size of cache */
		int balloc,	/* allocate data space for blocks? */
		int usize,	/* size of user data to alloc w/blks */     // ? @ fanglei
		int assoc,	/* associativity of cache */
		enum cache_policy policy,	/* replacement policy w/in sets */
		/* block access function, see description w/in struct cache def */
		unsigned int (*blk_access_fn) (enum mem_cmd cmd, md_addr_t baddr, int bsize, struct cache_blk_t * blk, tick_t now, int threadid), unsigned int hit_latency,	/* latency in cycles for a hit */
		int threadid)
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

	/* initialize cache stats */
	cp->hits = 0;
	cp->dhits = 0;
	cp->misses = 0;
	cp->dmisses = 0;
	cp->in_mshr = 0;
	cp->din_mshr = 0;
	cp->dir_access = 0;
	cp->data_access = 0;
	cp->coherence_misses = 0;
	cp->capacitance_misses = 0;
	cp->replacements = 0;
	cp->replInv = 0;
	cp->writebacks = 0;
	cp->wb_coherence_w = 0;
	cp->wb_coherence_r = 0;
	cp->invalidations = 0;
	cp->s_to_i = 0;
	cp->e_to_i = 0;
	cp->e_to_m = 0;
	cp->coherencyMisses = 0;
	cp->coherencyMissesOC = 0;
	cp->Invld = 0;
	cp->rdb = 0;
	cp->wrb = 0;
	cp->lastuse = 0;
	cp->dir_notification = 0;
	cp->Invalid_write_received = 0;
	cp->Invalid_write_received_hits = 0;
	cp->Invalid_Read_received = 0;
	cp->Invalid_Read_received_hits = 0;
	cp->ack_received = 0;

	/* blow away the last block accessed */
	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = -1;
	cp->last_blk = NULL;
	cp->threadid = threadid;

#ifdef	DCACHE_MSHR
	int mshr_flag = 0;
	if((!strcmp (cp->name, "dl1")) || (!strcmp (cp->name, "ul2")))
	{
		if(!strcmp (cp->name, "dl1"))
			mshr_flag = 0;
		else 
			mshr_flag = 1;	
		cp->mshr = (struct mshr_t *) calloc (1, sizeof (struct mshr_t));
		initMSHR(cp->mshr, mshr_flag);
		cp->mshr->threadid = threadid;
	}
	else
		cp->mshr = NULL;
#endif

	/* allocate data blocks */
	cp->data = (byte_t *) calloc (nsets * assoc, sizeof (struct cache_blk_t) + (cp->balloc ? (bsize * sizeof (byte_t)) : 0));
	if (!cp->data)
		fatal ("out of virtual memory");

	/* slice up the data blocks */
	for (bindex = 0, i = 0; i < nsets; i++)
	{
		cp->sets[i].way_head = NULL;
		cp->sets[i].way_tail = NULL;
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
#ifdef EUP_NETWORK
			blk->early_notification = 0;
#endif

			int m = 0, n=0;
			for(m=0; m<8; m++)              // ? @fanglei
			{
				for(n=0;n<8;n++)
					blk->dir_sharer[m][n]=0;
				blk->dir_state[m]=DIR_STABLE;
				blk->dir_dirty[m] = -1;
				blk->exclusive_time[m] = 0;
				if(MSI)
					blk->dir_blk_state[m] = MESI_INVALID;
				blk->ptr_cur_event[m] = NULL;
#ifdef SERIALIZATION_ACK
				blk->ReadPending[m] = -1;
				blk->ReadPending_event[m] = NULL;
				blk->pendingInvAck[m] = 0;
#endif
			}
			blk->ever_dirty = 0;
			blk->ever_touch = 0;

			blk->isStale = 0;

			blk->lineVolatile = 0;
			blk->isL1prefetch = 0;
			blk->prefetch_time = 0;

			blk->wordVolatile = 0;
			blk->wordInvalid = 0;
			blk->epochModified = -1;

			blk->user_data = (usize != 0 ? (byte_t *) calloc (usize, sizeof (byte_t)) : NULL);
			blk->invCause = 0;

			blk->ReadCount = 0;
			blk->WriteCount = 0;
			blk->WordCount = 0;
			blk->Type = 0;
			for(m=0;m<8;m++)
				blk->WordUseFlag[m] = 0;
			/* from garg */
			if (MSI)
			{
				blk->state = MESI_INVALID;
			}


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
#ifdef EUP_NETWORK
int UpgradeCheck(struct DIRECTORY_EVENT *event)
{
	int i, flag = 0, des = event->des1*mesh_size + event->des2;
	struct DIRECTORY_EVENT *temp;
	for(i=0;i<dir_fifo_num[des];i++)
	{
		temp = dir_fifo[des][(dir_fifo_head[des] + i)%DIR_FIFO_SIZE];
		if(temp->src1 == event->src1 && temp->src2 == event->src2 && temp->operation == WRITE_UPDATE && temp->popnetMsgNo < event->popnetMsgNo && (temp->addr>>cache_dl1[0]->set_shift == event->addr>>cache_dl1[0]->set_shift))
			flag = 1;
		if(event->operation == MISS_WRITE && temp->operation == WRITE_UPDATE && temp->popnetMsgNo < event->popnetMsgNo && (temp->addr>>cache_dl1[0]->set_shift == event->addr>>cache_dl1[0]->set_shift))
			flag = 1;
	}
	return flag;
}
int SameUpgradeCheck(struct DIRECTORY_EVENT *event)
{
	int i, j, flag = 0, des = (event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2;
	for(i = 0; i < INV_COLLECT_TABLE_SIZE; i++)
	{
		if(invCollectTable[des][i].isValid && (invCollectTable[des][i].addr>>cache_dl1[0]->set_shift == event->addr>>cache_dl1[0]->set_shift))
		{
			for(j=0;j<MAX_OWNERS;j++)
				if(invCollectTable[des][i].ownersList[j] == event->tempID)
					flag = 1;
		}
	} 
	return flag;
}
#endif
void allocate_free_event_list()
{
	int i;
	free_event_list = calloc(1, sizeof(struct DIRECTORY_EVENT));
	struct DIRECTORY_EVENT * next = free_event_list;

	for(i = 1; i < maxEvent; i++)
	{
		next->next = calloc(1, sizeof(struct DIRECTORY_EVENT));
		next = next->next;
		next->next = NULL;
	}
}

struct DIRECTORY_EVENT * allocate_event(int isSyncAccess)
{
	if(totalEventCount >= maxEvent)
		panic("Out of free events\n");
	struct DIRECTORY_EVENT * temp = free_event_list;
	free_event_list = free_event_list->next;
	temp->next = NULL;
	temp->op = 0;
	temp->isPrefetch = 0;			// if this is a prefetch
	temp->l2Status = 0;			// 0 - not yet accessed, 1 - l2 miss, 2 - l2 miss completed
	temp->l2MissStart = 0;
	temp->when = 0;                        	// when directory be accessed
	temp->started = 0;
	temp->mshr_time = 0;
	temp->transfer_time = 0;
	temp->req_time = 0;
	temp->store_cond = 0;
	temp->flip_flag = 0;
	temp->originalChildCount = 0;
#ifdef EXCLUSIVE_OPT
	temp->exclusive_after_wb = 0;
#endif
#ifdef SMD_USE_WRITE_BUF
	temp->l1_wb_entry = NULL;
#endif
#ifdef EUP_NETWORK
	temp->early_flag = 0;
#endif
	/* conflict reason */
	temp->input_buffer_full = 0;
	temp->pending_flag = 0;
	temp->L2miss_flag = 0;
	temp->dirty_flag = 0;
	temp->arrival_time = 0;
	temp->delay = 0;
	temp->data_reply = 0;
	temp->conf_bit = -1;
	temp->missNo = 0;
	temp->resend = 0;
	temp->operation = 0;                      	// directory operation, writehit update, writemiss search, readmiss search be different
	temp->tempID = 0;                         	// which processor issue the directory operation
	temp->addr = 0;                     	// address to access
	temp->blk1 = NULL;
	temp->blk_dir = NULL; 	// point to dl1 block accessed  

	temp->ptr_event = NULL;          	// point to the false entry in event queue
	temp->cp = NULL;                 	// cache to access
	temp->vp = NULL;                           	// ptr to buffer for input/output
	temp->nbytes = 0;                         	// number of bytes to access
	temp->udata = NULL;                     	// for return of user data ptr
	temp->rs = NULL;       
	temp->src1 = 0;
	temp->src2 = 0; 
	temp->des1 = 0; 
	temp->des2 = 0;
	temp->new_src1 = 0;
	temp->new_src2 = 0; 
	temp->new_des1 = 0; 
	temp->new_des2 = 0;
	temp->startCycle = 0;
	temp->parent = NULL;       // pointer to next entry
	temp->parent_operation = 0; 		// Parent operation is used for L2 cache MISS
	temp->childCount = 0; 
	temp->processingDone = 0; 
	temp->spec_mode = 0;
	temp->popnetMsgNo = 0;
	temp->eventType = 0;
	temp->rec_busy = 0;
	temp->sharer_num = 0;
	temp->prefetch_next = 0;
	temp->pendingInvAck = 0;
	temp->isReqL2Hit = 0;
	temp->isReqL2Inv = 0;
	temp->isReqL2SecMiss = 0;
	temp->isReqL2Trans = 0;
	temp->reqNetTime = 0;
	temp->reqAtDirTime = 0;
	temp->reqAckTime = 0;
	temp->isSyncAccess = isSyncAccess;
	totalEventCount++;
	totaleventcount++;
	temp->totaleventcount = totaleventcount;
	return temp;
}

void free_event(struct DIRECTORY_EVENT *event)
{
	event->next = free_event_list;
	free_event_list = event;
	totalEventCount--;
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
		struct stat_sdb_t *sdb, int id)	/* stats database */
{
	char buf[512], buf1[512], *name;


	if (cp->threadid != -1 && cp->threadid != id)
		panic ("Wrong cache being called in cache_reg_stats\n");

	/* get a name for this cache */
	if (!cp->name || !cp->name[0])
		name = "<unknown>";
	else
		name = cp->name;

	name = mystrdup(cp->name);

	sprintf(buf, "_%d", id);
	strcat(name, buf);

	sprintf (buf, "%s.accesses", name);
	sprintf (buf1, "%s.hits + %s.misses + %s.in_mshr", name, name, name);
	stat_reg_formula (sdb, buf, "total number of accesses", buf1, "%12.0f");
	sprintf (buf, "%s.hits", name);
	stat_reg_counter (sdb, buf, "total number of hits", &cp->hits, 0, NULL);
	sprintf (buf, "%s.in_mshr", name);
	stat_reg_counter (sdb, buf, "total number of secondary misses", &cp->in_mshr, 0, NULL);
	sprintf (buf, "%s.misses", name);
	stat_reg_counter (sdb, buf, "total number of misses", &cp->misses, 0, NULL);

	sprintf (buf, "%s.daccesses", name);
	sprintf (buf1, "%s.dhits + %s.dmisses + %s.din_mshr", name, name, name);
	stat_reg_formula (sdb, buf, "total number of data accesses", buf1, "%12.0f");
	sprintf (buf, "%s.dhits", name);
	stat_reg_counter (sdb, buf, "total number of data hits", &cp->dhits, 0, NULL);
	sprintf (buf, "%s.din_mshr", name);
	stat_reg_counter (sdb, buf, "total number of data secondary misses", &cp->din_mshr, 0, NULL);
	sprintf (buf, "%s.dmisses", name);
	stat_reg_counter (sdb, buf, "total number of data misses", &cp->dmisses, 0, NULL);

	sprintf (buf, "%s.dir_access", name);
	stat_reg_counter (sdb, buf, "total number of dir_access", &cp->dir_access, 0, NULL);
	sprintf (buf, "%s.data_access", name);
	stat_reg_counter (sdb, buf, "total number of data_access", &cp->data_access, 0, NULL);
	sprintf (buf, "%s.coherence_misses", name);
	stat_reg_counter (sdb, buf, "total number of misses due to invalidation", &cp->coherence_misses, 0, NULL);
	sprintf (buf, "%s.capacitance_misses", name);
	stat_reg_counter (sdb, buf, "total number of misses due to capacitance", &cp->capacitance_misses, 0, NULL);
	sprintf (buf, "%s.replacements", name);
	stat_reg_counter (sdb, buf, "total number of replacements", &cp->replacements, 0, NULL);
	sprintf (buf, "%s.replInv", name);
	stat_reg_counter (sdb, buf, "total number of replacements which also include invalidations", &cp->replInv, 0, NULL);
	sprintf (buf, "%s.writebacks", name);
	stat_reg_counter (sdb, buf, "total number of writebacks", &cp->writebacks, 0, NULL);
	sprintf (buf, "%s.wb_coherence_w", name);
	stat_reg_counter (sdb, buf, "total number of writebacks due to coherence write", &cp->wb_coherence_w, 0, NULL);
	sprintf (buf, "%s.wb_coherence_r", name);
	stat_reg_counter (sdb, buf, "total number of writebacks due to coherence read", &cp->wb_coherence_r, 0, NULL);
	sprintf (buf, "%s.Invld", name);
	stat_reg_counter (sdb, buf, "total number of Invld", &cp->Invld, 0, NULL);
	sprintf (buf, "%s.invalidations", name);
	stat_reg_counter (sdb, buf, "total number of invalidations", &cp->invalidations, 0, NULL);
	sprintf (buf, "%s.s_to_i", name);
	stat_reg_counter (sdb, buf, "total number of shared to invalid(invalidation_received_sub)", &cp->s_to_i, 0, NULL);
	sprintf (buf, "%s.e_to_i", name);
	stat_reg_counter (sdb, buf, "total number of exclusive to invalid(invalidation_received_sub)", &cp->e_to_i, 0, NULL);
	sprintf (buf, "%s.e_to_m", name);
	stat_reg_counter (sdb, buf, "total number of exclusive to invalid(invalidation_received_sub)", &cp->e_to_m, 0, NULL);
	/* how many misses waiting for MSHR entry */
	sprintf (buf, "%s.dir_notification", name);
	stat_reg_counter (sdb, buf, "total number of updating directory", &cp->dir_notification, 0, NULL);
	sprintf (buf, "%s.Invalid_write_received", name);
	stat_reg_counter (sdb, buf, "total number of invalidation write received", &cp->Invalid_write_received, 0, NULL);
	sprintf (buf, "%s.Invalid_read_received", name);
	stat_reg_counter (sdb, buf, "total number of invalidation read received", &cp->Invalid_Read_received, 0, NULL);
	sprintf (buf, "%s.Invalid_w_received_hits", name);
	stat_reg_counter (sdb, buf, "total number of invalidation write received_hits", &cp->Invalid_write_received_hits, 0, NULL);
	sprintf (buf, "%s.Invalid_r_received_hits", name);
	stat_reg_counter (sdb, buf, "total number of invalidation read received_hits", &cp->Invalid_Read_received_hits, 0, NULL);
	sprintf (buf, "%s.acknowledgement received", name);
	stat_reg_counter (sdb, buf, "total number of acknowledgement received", &cp->ack_received, 0, NULL);

	sprintf (buf, "%s.coherencyMisses", name);
	stat_reg_counter (sdb, buf, "total number of coherency Misses", &cp->coherencyMisses, 0, NULL);
	sprintf (buf, "%s.coherencyMissesOC", name);
	stat_reg_counter (sdb, buf, "total number of coherency Misses due to optimistic core", &cp->coherencyMissesOC, 0, NULL);
	sprintf (buf, "%s.miss_rate", name);
	sprintf (buf1, "%s.misses / %s.accesses", name, name);
	stat_reg_formula (sdb, buf, "miss rate (i.e., misses/ref)", buf1, NULL);
	sprintf (buf, "%s.repl_rate", name);
	sprintf (buf1, "%s.replacements / %s.accesses", name, name);
	stat_reg_formula (sdb, buf, "replacement rate (i.e., repls/ref)", buf1, NULL);
	sprintf (buf, "%s.wb_rate", name);
	sprintf (buf1, "%s.writebacks / %s.accesses", name, name);
	stat_reg_formula (sdb, buf, "writeback rate (i.e., wrbks/ref)", buf1, NULL);
	sprintf (buf, "%s.inv_rate", name);
	sprintf (buf1, "%s.invalidations / %s.accesses", name, name);
	stat_reg_formula (sdb, buf, "invalidation rate (i.e., invs/ref)", buf1, NULL);


	sprintf (buf, "%s.flushCount", name);
	stat_reg_counter (sdb, buf, "total flushes", &cp->flushCount, 0, NULL);
	sprintf (buf, "%s.lineFlushed", name);
	stat_reg_counter (sdb, buf, "lines flushed", &cp->lineFlushCount, 0, NULL);

	sprintf (buf, "%s.flushRate", name);
	sprintf (buf1, "%s.lineFlushed / %s.flushCount", name, name);
	stat_reg_formula (sdb, buf, "flush rate (i.e., lines/chunk)", buf1, NULL);

	free(name);
}

/* ______________________________ start hot leakage **************************************/






/* ______________________________ end hot leakage **************************************/

/* print cache stats */
	void
cache_stats (struct cache_t *cp,	/* cache instance */
		FILE * stream)	/* output stream */
{
	double sum = (double) (cp->hits + cp->misses);

	fprintf (stream, "cache: %s: %.0f hits %.0f misses %.0f repls %.0f invalidations\n", cp->name, (double) cp->hits, (double) cp->misses, (double) cp->replacements, (double) cp->invalidations);
	fprintf (stream, "cache: %s: miss rate=%f  repl rate=%f  invalidation rate=%f\n", cp->name, (double) cp->misses / sum, (double) (double) cp->replacements / sum, (double) cp->invalidations / sum);
}

	int
bank_check (md_addr_t addr, struct cache_t *cp)
{
	return 0;
}

#define BUS_TRAN_LAT 0

#define SET 1
#define TAG 2


/********************************************multiprocessor************************************************/
/*hq: my counter */
 counter_t launch_sp_access ;
 counter_t l1launch_sp_access ;
void dir_eventq_insert(struct DIRECTORY_EVENT *event);     // event queue directory version, put dir event in queue
#ifdef DCACHE_MSHR
struct DIRECTORY_EVENT *event_created = NULL;
#endif
#ifdef L1_STREAM_PREFETCHER
	void
l1init_sp ()
{
	int i, j;
	for(j=0;j<numcontexts;j++)
	{
		for (i = 0; i < MISS_TABLE_SIZE; i++)
			l1miss_history_table[j].history[i] = 0;
		l1miss_history_table[j].mht_tail = 0;
		l1miss_history_table[j].mht_num = 0;

		for (i = 0; i < STREAM_ENTRIES; i++)
		{
			l1stream_table[j][i].valid = 0;
			l1stream_table[j][i].addr = 0;
			l1stream_table[j][i].stride = 0;
			l1stream_table[j][i].remaining_prefetches = 0;
			l1stream_table[j][i].last_use = 0;
		}
	}
}

	void
l1insert_sp (md_addr_t addr, int threadid)
{                               /* This function is called if L2 cache miss is discovered. */
	int i, num;
	int dist;
	int min_entry;
	md_addr_t stride_addr;
	int count = 0;

	if (l1miss_history_table[threadid].mht_num == 0)
	{                           /* Table is empty */
		l1miss_history_table[threadid].history[l1miss_history_table[threadid].mht_tail] = (addr & ~(cache_dl1[0]->bsize - 1));
		l1miss_history_table[threadid].mht_tail = (l1miss_history_table[threadid].mht_tail + 1) % MISS_TABLE_SIZE;
		l1miss_history_table[threadid].mht_num++;
		return;
	}

	dist = (addr & ~(cache_dl1[0]->bsize - 1)) - l1miss_history_table[threadid].history[(l1miss_history_table[threadid].mht_tail + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE];
	stride_addr = l1miss_history_table[threadid].history[(l1miss_history_table[threadid].mht_tail + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE];
	min_entry = (l1miss_history_table[threadid].mht_tail + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE;

	/* First calculate the minimum DELTA */
	for (num = 1, i = (l1miss_history_table[threadid].mht_tail + MISS_TABLE_SIZE - 2) % MISS_TABLE_SIZE; num < l1miss_history_table[threadid].mht_num; i = (i + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE, num++)
	{
		int curr_dist = (addr & ~(cache_dl1[0]->bsize - 1)) - l1miss_history_table[threadid].history[i];

		if (abs (curr_dist) < abs (dist))
		{
			dist = curr_dist;
			stride_addr = l1miss_history_table[threadid].history[i];
			min_entry = i;
		}
	}

	i = min_entry;
	do
	{
		i = (i + MISS_TABLE_SIZE - 1) % MISS_TABLE_SIZE;
		int curr_dist = (stride_addr & ~(cache_dl1[0]->bsize - 1)) - l1miss_history_table[threadid].history[i];

		if (curr_dist == dist)
		{
			count++;
			stride_addr = l1miss_history_table[threadid].history[i];
		}
	}
	while (i != l1miss_history_table[threadid].mht_tail);

	/* Now we can insert this miss entry into the miss history table */
	l1miss_history_table[threadid].history[l1miss_history_table[threadid].mht_tail] = (addr & ~(cache_dl1[0]->bsize - 1));
	l1miss_history_table[threadid].mht_tail = (l1miss_history_table[threadid].mht_tail + 1) % MISS_TABLE_SIZE;
	if (l1miss_history_table[threadid].mht_num < MISS_TABLE_SIZE)
		l1miss_history_table[threadid].mht_num++;

	if (count > 1)
	{                           /* We hit a stride. Insert the stride into stream table. Also initiate the first prefetch.
				       If the table is completely full, use LRU algorithm to evict one entry. */
		int id = 0;

		if (l1stream_table[threadid][0].valid == 1)
		{
			int last_use = l1stream_table[threadid][0].last_use;

			id = 0;
			for (i = 1; i < STREAM_ENTRIES; i++)
			{
				if (l1stream_table[threadid][i].valid == 1)
				{
					if (last_use < l1stream_table[threadid][i].last_use)
					{
						last_use = l1stream_table[threadid][i].last_use;
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
		l1stream_table[threadid][id].valid = 1;
		l1stream_table[threadid][id].addr = addr & ~(cache_dl1[0]->bsize - 1);
		l1stream_table[threadid][id].stride = dist;
		l1stream_table[threadid][id].remaining_prefetches = PREFETCH_DISTANCE;
		l1stream_table[threadid][id].last_use = 2000000000;

		/* Now call a prefetch for this addr */
		l1launch_sp (addr, threadid);
	}
}

	void
l1launch_sp (md_addr_t addr, int threadid)
{                               /* This function is called when access in L2 to pre-fetch data is discovered. */
	int i;
	l1launch_sp_access++;/* hq: my counter */

	for (i = 0; i < STREAM_ENTRIES; i++)
	{
		if (l1stream_table[threadid][i].valid == 1 && l1stream_table[threadid][i].addr == (addr & ~(cache_dl1[0]->bsize - 1)))
		{
			int load_lat = 0;
			byte_t *l1cache_line = calloc (cache_dl1[0]->bsize, sizeof (byte_t));

			l1stream_table[threadid][i].addr = l1stream_table[threadid][i].addr + l1stream_table[threadid][i].stride;
			l1stream_table[threadid][i].remaining_prefetches--;
			l1stream_table[threadid][i].last_use = sim_cycle;

			if (l1stream_table[threadid][i].remaining_prefetches == 1)
			{
				l1stream_table[threadid][i].valid = 0;
			}

			/* Tell the system that this call at this time is a prefetch, so stream
			 ** prefetch must not trigger.*/
			dl1_prefetch_active = 1;
			if(l1stream_table[threadid][i].stride != 0)
			{
				md_addr_t addr_prefetch = (l1stream_table[threadid][i].addr & ~(cache_dl1[0]->bsize - 1));
				md_addr_t tag = CACHE_TAG (cache_dl1[threadid], addr_prefetch);
				md_addr_t set = CACHE_SET (cache_dl1[threadid], addr_prefetch);
				struct cache_blk_t *blk;
				int cache_hit_flag = 0;
				
				for (blk = cache_dl1[threadid]->sets[set].way_head; blk; blk = blk->way_next)
				{
					if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
						cache_hit_flag = 1;
				}
				if(cache_hit_flag == 0)
				{
					int src = (addr_prefetch >> cache_dl2->set_shift) % numcontexts;
					total_L1_prefetch ++;	   

					int matchnum;
					matchnum = MSHR_block_check(cache_dl1[threadid]->mshr, addr_prefetch, cache_dl1[threadid]->set_shift);
					if(!matchnum && (cache_dl1[threadid]->mshr->freeEntries > 1))
					{
						struct DIRECTORY_EVENT *new_event = allocate_event(0);
						if(new_event == NULL)       panic("Out of Virtual Memory");
						new_event->data_reply = 0;
						new_event->req_time = 0;
						new_event->pending_flag = 0;
						//new_event->op = LDQ;
						new_event->op = LW; //wxh
						new_event->isPrefetch = 0;
#ifdef PREFETCH_OPTI  
						new_event->operation = L1_PREFETCH;
#else
						new_event->operation = MISS_READ;
#endif
						new_event->src1 = threadid/mesh_size+MEM_LOC_SHIFT; 
						/* mem_loc_shift is for memory model because memory controller are allocated at two side of the chip */
						new_event->src2 = threadid%mesh_size;
#ifdef CENTRALIZED_L2
						//the centralized L2 has CENTL2_BANK_NUM banks, and each of them has one NI (network interface). 
						src = (addr_prefetch >> cache_dl2->set_shift) % CENTL2_BANK_NUM;
						new_event->des1 = mesh_size + 2 ;    //This indicates that its a L2 bank access, the location of this L2 bank NI is on the bottom of the chip, (In fact, it can be anywhere) 
						new_event->des2 = src; //this indicate the bank id
#else
						new_event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
						new_event->des2 = (src %mesh_size);
#endif
						new_event->processingDone = 0;
						new_event->startCycle = sim_cycle;
						new_event->parent = NULL;
						new_event->tempID = threadid;
						new_event->resend = 0;
						new_event->blk_dir = NULL;
						new_event->cp = cache_dl1[threadid];
						new_event->addr = addr_prefetch;
						new_event->vp = NULL;
						new_event->nbytes = cache_dl1[threadid]->bsize;
						new_event->udata = NULL;
						new_event->cmd = Write;
						new_event->rs = NULL;
						new_event->started = sim_cycle;
						new_event->spec_mode = 0;
						new_event->L2miss_flag = 0;
						if(l1stream_table[threadid][i].remaining_prefetches == PREFETCH_DISTANCE - 1)
						{
							new_event->prefetch_next = 2;	/* first prefetch packet */
							total_L1_first_prefetch ++;	   
						}
						else
						{
							new_event->prefetch_next = 4;	
							total_L1_sec_prefetch ++;	   
						}
						new_event->conf_bit = -1;
						new_event->input_buffer_full = 0;

						scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, 0);

						dir_eventq_insert(new_event);
						event_created = new_event;
						MSHRLookup(cache_dl1[threadid]->mshr, addr_prefetch, WAIT_TIME, 0, NULL);
					}
				}
			}

			dl1_prefetch_active = 0;
			free (l1cache_line);
			break;
		}
	}
}

#endif
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
insert_sp (md_addr_t addr, int threadid)
{                               /* This function is called if L2 cache miss is discovered. */
	int i, num;
	int dist;
	int min_entry;
	md_addr_t stride_addr;
	int count = 0;

	if (miss_history_table.mht_num == 0)
	{                           /* Table is empty */
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
	{                           /* We hit a stride. Insert the stride into stream table. Also initiate the first prefetch.
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
		launch_sp (id, threadid);
	}
}

	void
launch_sp (int id, int threadid)
{                               /* This function is called when access in L2 to pre-fetch data is discovered. */
	int i = id;
    launch_sp_access++;/* hq: my counter */

	if (stream_table[i].valid == 1)
	{
		for (; stream_table[i].remaining_prefetches; stream_table[i].remaining_prefetches --)
		{
			int load_lat = 0;
			byte_t *l1cache_line = calloc (cache_dl2->bsize, sizeof (byte_t));

			stream_table[i].addr = stream_table[i].addr + stream_table[i].stride;
			stream_table[i].last_use = sim_cycle;


			/* Tell the system that this call at this time is a prefetch, so stream
			 ** prefetch must not trigger.*/
			dl2_prefetch_active = 1;
			dl2_prefetch_id = i;
			struct DIRECTORY_EVENT *event = allocate_event(0);
			if(event == NULL)       panic("Out of Virtual Memory");
			event->conf_bit = -1;
			event->pending_flag = 0;
			event->addr = (stream_table[i].addr & ~(cache_dl2->bsize - 1));
			event->prefetch_next = 3;
			event->started = sim_cycle;
			event->operation = L2_PREFETCH;
			event->startCycle = sim_cycle;
			event->op = 0;
			event->cp = cache_dl1[0];
			event->tempID = threadid;
			event->nbytes = cache_dl1[0]->bsize;
			event->udata = NULL;
			event->rs = NULL; 
			event->spec_mode = 0;
			event->input_buffer_full = 0;
			event->src1 = threadid/mesh_size + MEM_LOC_SHIFT;
			event->src2 = threadid%mesh_size;
			long src;
#ifdef CENTRALIZED_L2
			//the centralized L2 has CENTL2_BANK_NUM banks, and each of them has one NI (network interface). 
			src = (event->addr >> cache_dl2->set_shift) % CENTL2_BANK_NUM;
			event->des1 = mesh_size + 2 ;    //This indicates that its a L2 bank access, the location of this L2 bank NI is on the bottom of the chip, (In fact, it can be anywhere) 
			event->des2 = src; //this indicate the bank id
#else
			src = (event->addr >> cache_dl2->set_shift) % numcontexts;
			event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
			event->des2 = (src %mesh_size);
#endif
			event->data_reply = 0;
			event->processingDone = 0;
			event->req_time = 0;
			dir_eventq_insert(event);

			dl2_prefetch_active = 0;
			free (l1cache_line);
		}
	}
	stream_table[i].remaining_prefetches = 1;
}

#endif


extern counter_t totalL2WriteReqHits;
extern counter_t totalL2WriteReqNoSharer;
extern counter_t totalL2WriteReqDirty;
extern counter_t delayL2WriteReqDirty;
extern counter_t totalL2WriteReqShared;
extern counter_t delayL2WriteReqShared;

extern counter_t totalL2ReadReqHits;
extern counter_t totalL2ReadReqNoSharer;
extern counter_t totalL2ReadReqDirty;
extern counter_t delayL2ReadReqDirty;

extern counter_t totalL2ReadReqShared;
extern counter_t totalL2WriteReqSharedWT;
extern counter_t totalL2WriteReqDirtyWT;
extern counter_t totalL2ReadReqDirtyWT;

extern counter_t totalL1LinePrefUse;
extern counter_t totalL1LinePrefNoUse;
extern counter_t totalL1LineReadUse;
extern counter_t totalL1LineReadNoUse;
extern counter_t totalL1LineWriteUse;
extern counter_t totalL1LineWriteNoUse;

extern counter_t totalL1LineExlUsed;
extern counter_t totalL1LineExlInv;
extern counter_t totalL1LineExlDrop;

extern counter_t    totalL2ReqPrimMiss;
extern counter_t    totalL2ReqSecMiss;
extern counter_t    totalL2ReqHit;
extern counter_t    totalL2ReqInTrans;
extern counter_t    totalL2OwnReqInTrans;
extern counter_t    totalL2ReqInInv;

extern counter_t    delayL2ReqPrimMiss;
extern counter_t    delayL2ReqSecMiss;
extern counter_t    delayL2ReqHit;
extern counter_t    delayL2ReqInTrans;
extern counter_t    delayL2OwnReqInTrans;
extern counter_t    delayL2ReqInInv;

extern counter_t	totalWriteReq;
extern counter_t	totalWriteReqInv;
extern counter_t	totalWriteDelay;
extern counter_t	totalWriteInvDelay;
extern counter_t	totalUpgradeReq;
counter_t	totalUpgradeMiss;
extern counter_t	totalUpgradeReqInv;
extern counter_t	totalUpgradeDelay;
extern counter_t	totalUpgradeInvDelay;
counter_t m_update_miss[MAXTHREADS];

extern counter_t countNonAllocReadEarly;
counter_t sharer_distr[MAXTHREADS];
counter_t total_corner_packets, total_neighbor_packets, total_far_packets;

#ifdef SERIALIZATION_ACK
int isInvCollectTableFull(int threadid)
{
	int i;
	for(i = 0; i < INV_COLLECT_TABLE_SIZE; i++)
	{
		if(!invCollectTable[threadid][i].isValid)
			return 0;
	}
	return 1;
}

void newInvCollect(int threadid, md_addr_t addr, int pendingInv, int owner, int conf)
{
	int i;

	/* first check if there is already an entry for this address */
	for(i = 0; i < INV_COLLECT_TABLE_SIZE; i++)
	{
		if(invCollectTable[threadid][i].isValid && invCollectTable[threadid][i].addr == addr)
			panic("Error in newInvCollect: address already exists.");
	}

	/* Look for free entry */
	for(i = 0; i < INV_COLLECT_TABLE_SIZE; i++)
	{
		if(!invCollectTable[threadid][i].isValid)
		{
			invCollectTable[threadid][i].isValid = 1;
			invCollectTable[threadid][i].addr = addr;
			invCollectTable[threadid][i].pendingInv = pendingInv;
			invCollectTable[threadid][i].numOwners = 1;
			invCollectTable[threadid][i].ownersList[0] = owner;
			invCollectTable[threadid][i].ack_conf[0] = conf;
			return;
		}
	}
	panic("Error in newInvCollect: table is full!!!");
}

int addOwner(int threadid, md_addr_t addr, int owner)
{
	int i;
	for(i = 0; i < INV_COLLECT_TABLE_SIZE; i++)
	{
		if(invCollectTable[threadid][i].isValid && invCollectTable[threadid][i].addr == addr)
		{
			if(invCollectTable[threadid][i].numOwners >= MAX_OWNERS)
				return 0;
			invCollectTable[threadid][i].ownersList[invCollectTable[threadid][i].numOwners] = owner;
			invCollectTable[threadid][i].numOwners++;
			return 1;
		}
	}
	panic("Error in addOwner: entry not found!!!");
}

//void scheduleThroughNetwork(struct DIRECTORY_EVENT *event, counter_t start, int type, int vc);

int invAckUpdate(int threadid, md_addr_t addr, int owner, int isSyncAccess)
{
	int i;
	for(i = 0; i < INV_COLLECT_TABLE_SIZE; i++)
	{
		//if(invCollectTable[threadid][i].isValid && invCollectTable[threadid][i].addr == addr )
		if(invCollectTable[threadid][i].isValid && invCollectTable[threadid][i].addr == addr && invCollectTable[threadid][i].ownersList[invCollectTable[threadid][i].numOwners - 1] == owner)
		{
			invCollectTable[threadid][i].pendingInv--;
			if(invCollectTable[threadid][i].pendingInv == 0)
			{/* send out final acknowledgements to all the owners and release this entry */
				int j = 0;
				for(j = 0; j < invCollectTable[threadid][i].numOwners; j++)
				{
					struct DIRECTORY_EVENT *new_event = allocate_event(isSyncAccess);

					if(new_event == NULL) 	panic("Out of Virtual Memory");
					new_event->req_time = 0;
					new_event->conf_bit = -1;
					new_event->input_buffer_full = 0;
					new_event->pending_flag = 0;
					new_event->op = 0;
					new_event->isPrefetch = 0;
					new_event->L2miss_flag = 0;
					new_event->operation = FINAL_INV_ACK;
					new_event->src1 = threadid/mesh_size+MEM_LOC_SHIFT;
					new_event->src2 = threadid%mesh_size;
					new_event->des1 = invCollectTable[threadid][i].ownersList[j]/mesh_size+MEM_LOC_SHIFT;
					new_event->des2 = invCollectTable[threadid][i].ownersList[j]%mesh_size;
					new_event->processingDone = 0;
					new_event->startCycle = sim_cycle;
					new_event->parent = NULL;
					new_event->tempID = invCollectTable[threadid][i].ownersList[j];
					new_event->resend = 0;
					new_event->blk_dir = NULL;
					new_event->cp = NULL;
					new_event->addr = addr;
					new_event->vp = NULL;
					new_event->nbytes = 0;
					new_event->udata = NULL;
					new_event->cmd = Write;
					new_event->rs = NULL;
					new_event->started = sim_cycle;
					new_event->spec_mode = 0;
					if(invCollectTable[threadid][i].ack_conf[j])
						new_event->conf_bit = 1;  //FIXME: using confirmation bit to transfer the final acknowledgement 
					//scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size);	
					int vc = 0;
					if (!mystricmp (network_type, "MESH"))
						vc = popnetBufferSpace(new_event->src1, new_event->src2, -1);
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);	
					dir_eventq_insert(new_event);
				}
				invCollectTable[threadid][i].isValid = 0;
				return 1;
			}
			return 2;
		}
	}
	return 0;
}
#endif

extern counter_t invalidation_replay[MAXTHREADS];

void sendLSQInv(md_addr_t addr, int threadid)
{
	if (collect_stats)
	{
		int cx, dx;
		extern int LSQ_size;
		context *current = thecontexts[threadid];
		md_addr_t mask;

		mask = ~cache_dl1[threadid]->blk_mask;

		for (cx = current->LSQ_head, dx = 0; dx != current->LSQ_num; cx = (cx + 1) % LSQ_size, dx++)
		{	
			if ((MD_OP_FLAGS (current->LSQ[cx].op) & (F_MEM | F_LOAD)) == (F_MEM | F_LOAD) && (current->LSQ[cx].addr & mask) == (addr & mask) && !current->LSQ[cx].spec_mode && !current->LSQ[cx].isPrefetch && current->LSQ[cx].issued)
			{
#ifdef SEQUENTIAL_CONSISTENCY
				fixSpecState (current->LSQ[cx].prod[1]->index, threadid);
				current->regs = current->LSQ[cx].prod[1]->backupReg;
				seqConsistancyReplay (current->LSQ[cx].prod[1]->index, threadid);
				invalidation_replay[threadid]++;
				break;
#endif
#ifdef RELAXED_CONSISTENCY
				current->LSQ[cx].invalidationReceived = 1;
#endif
			}
		}
	}
}

/*		DIR FIFO IMPLEMENTATION			*/
void reset_ports()
{
	int i;
	for(i = 0; i < MAXTHREADS+mesh_size*2; i++)
	{
		dir_fifo_portuse[i] = 0;
		l1_fifo_portuse[i] = 0;
#ifdef TSHR
		tshr_fifo_portuse[i] = 0;
#endif
	}
}
#ifdef LOCK_CACHE_REGISTER
void LockRegesterCheck(struct DIRECTORY_EVENT *event)
{ /* update the lock register if necessary */
	int threadid = (event->src1-MEM_LOC_SHIFT)*mesh_size + event->src2;
	struct LOCK_CACHE *LockReg = &Lock_cache[threadid];
	if(LockReg->SubScribeVect == 0 && LockReg->lock_owner == 0)
	{/*The lock register is ready to be replaced or update */
		LockReg->SubScribeVect |= (unsigned long long int)1<<event->tempID;
		LockReg->tag = event->addr & ~(cache_dl1[0]->bsize - 1);
	}
	else if((event->addr & ~(cache_dl1[0]->bsize - 1)) == LockReg->tag)  /*Someone has already update the lock register*/
		LockReg->SubScribeVect |= (unsigned long long int)1<<event->tempID;
	return;
}
int lock_cache_access(struct DIRECTORY_EVENT *event)
{
	int threadid = (event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2;
	if(event->store_cond == MY_STL_C || event->store_cond == MY_LDL_L)
	{
		if(event->operation == INV_MSG_UPDATE)
		{
			if(event->addr == common_regs_s[thecontexts[threadid]->masterid][threadid].address && common_regs_s[thecontexts[threadid]->masterid][threadid].regs_lock)
			{
				if(lock_fifo_num[threadid] >= MAXLOCKEVENT)
				{
					lock_fifo_full ++;
					return 0;
				}
				lock_fifo[threadid][lock_fifo_tail[threadid]] = event;
				lock_fifo_num[threadid] ++;
				lock_fifo_tail[threadid] = (lock_fifo_tail[threadid] + 1 + MAXLOCKEVENT) % MAXLOCKEVENT;
				return 1;
			}
		}
		if(((event->addr & ~(cache_dl1[0]->bsize - 1)) == Lock_cache[threadid].tag))
		{
			if(lock_fifo_num[threadid] >= MAXLOCKEVENT)
			{
				lock_fifo_full ++;
				return 0;
			}
			lock_fifo[threadid][lock_fifo_tail[threadid]] = event;
			lock_fifo_num[threadid] ++;
			lock_fifo_tail[threadid] = (lock_fifo_tail[threadid] + 1 + MAXLOCKEVENT) % MAXLOCKEVENT;
			return 1;
		}
	}
	return 0;
}
int lock_cache_operation(struct DIRECTORY_EVENT *event)
{ /* lock register accesses */
	int threadid = (event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2;
	struct LOCK_CACHE *LockReg = &Lock_cache[threadid];
	int Threadid; /*Other threadid*/

	switch (event->store_cond)
	{
		case MY_LDL_L:
			if((event->addr & ~(cache_dl1[0]->bsize - 1)) == LockReg->tag)
			{ /* Hit in Lock register */
				lock_cache_hit ++;
				LockReg->SubScribeVect |= (unsigned long long int)1<<event->tempID; /* subscribe this thread to this subscribe vector for update used */
				/* FIXME: read the bool value from the BoolValue and send it back */
				event->operation = ACK_DIR_READ_SHARED;
				event->src1 += event->des1;
				event->des1 = event->src1 - event->des1;
				event->src1 = event->src1 - event->des1;
				event->src2 += event->des2;
				event->des2 = event->src2 - event->des2;
				event->src2 = event->src2 - event->des2;
				event->processingDone = 0;
				event->startCycle = sim_cycle;
				event->parent = NULL;
				event->blk_dir = NULL;
				event->arrival_time = sim_cycle - event->arrival_time;
				scheduleThroughNetwork(event, sim_cycle, data_packet_size, 0);
				dir_eventq_insert(event);
				return 1;
			}
			else 
			{ /* miss in Lock Register */
				lock_cache_miss ++;
				/* access L2 and directory */
				panic("LOCK OPERATION!");
			}
		break;
		case MY_STL_C:
			if(event->operation == INV_MSG_UPDATE)
			{
				if(event->addr == common_regs_s[thecontexts[threadid]->masterid][threadid].address && common_regs_s[thecontexts[threadid]->masterid][threadid].regs_lock)
				{ /*update the value in the load link register and also put the load link flag to 0, write through the new value to L1 cache, FIXME*/
					common_regs_s[thecontexts[threadid]->masterid][threadid].regs_lock = 0;
					common_regs_s[thecontexts[threadid]->masterid][threadid].subscribe_value = ~common_regs_s[thecontexts[threadid]->masterid][threadid].subscribe_value;
					/*fail the store conditional if there is one */
					int i, mshr_size;
					mshr_size = cache_dl1[threadid]->mshr->mshrSIZE;
					for (i=0;i<mshr_size;i++)
					{
						if(cache_dl1[threadid]->mshr->mshrEntry[i].isValid 
								&& ((cache_dl1[threadid]->mshr->mshrEntry[i].addr >> cache_dl1[threadid]->set_shift) == (event->addr >> cache_dl1[threadid]->set_shift))) 
						{
							cache_dl1[threadid]->mshr->mshrEntry[i].endCycle = sim_cycle;
							if(!event->spec_mode && cache_dl1[threadid]->mshr->mshrEntry[i].event->rs)
							{
								cache_dl1[threadid]->mshr->mshrEntry[i].event->rs->writewait = 2;
								cache_dl1[threadid]->mshr->mshrEntry[i].event->rs->STL_C_fail = 1;
							}
#ifdef SMD_USE_WRITE_BUF
							else if(event->l1_wb_entry)
								cache_dl1[threadid]->mshr->mshrEntry[i].event->l1_wb_entry->STL_C_fail = 1;
#endif
							break;
						}
					}
					free_event(event);
					return 1;
				}
				panic("The store conditional update did not match the load link address register!");
			}
			else if(event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
			{
				if((event->addr & ~(cache_dl1[0]->bsize - 1)) == Lock_cache[threadid].tag)
				{ /* Hit in Lock register */
					lock_cache_hit ++;
					if(LockReg->lock_owner != 0 && LockReg->lock_owner != event->tempID+1) /*Someone has already have the lock, this store conditional fail*/
					{ /*drop the store conditional message */
						printf("threadid %d fail on lock \n", event->tempID);
						free_event(event);
						return 1;
					}
					else
					{ /*first store condition acquire the lock*/
						LockReg->SubScribeVect &= ~(unsigned long long int)1<<event->tempID; /* unsubscribe this thread to this subscribe vector*/
						/* change the bool value, write through L2 cache FIXME*/
						LockReg->BoolValue = ~LockReg->BoolValue;
						if(LockReg->lock_owner == 0)
						{
							LockReg->lock_owner = 1+event->tempID;  /*Indicate it is the lock owner */
							printf("threadid %d acquire on lock \n", event->tempID);
						}
						else if(LockReg->lock_owner == 1+event->tempID)
						{
							LockReg->lock_owner = 0; /* inticate this thread release the lock */
							printf("threadid %d release on lock \n", event->tempID);
						}

						/* update other subscribers */
						for (Threadid = 0; Threadid < numcontexts; Threadid++)
						{
							if((((LockReg->SubScribeVect) & ((unsigned long long int)1 << (Threadid%64))) == ((unsigned long long int)1 << (Threadid%64))) && (Threadid != event->tempID))
							{
								struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
								if(new_event == NULL) 	panic("Out of Virtual Memory");
								new_event->req_time = 0;
								new_event->conf_bit = -1;
								new_event->input_buffer_full = 0;
								new_event->pending_flag = 0;
								new_event->op = event->op;
								new_event->isPrefetch = 0;
								new_event->L2miss_flag = event->L2miss_flag;
								new_event->operation = INV_MSG_UPDATE;
								new_event->conf_bit = event->childCount;
								new_event->src1 = threadid/mesh_size+MEM_LOC_SHIFT;
								new_event->src2 = threadid%mesh_size;
								new_event->des1 = Threadid/mesh_size+MEM_LOC_SHIFT;
								new_event->des2 = Threadid%mesh_size;
								new_event->processingDone = 0;
								new_event->startCycle = sim_cycle;
								new_event->parent = event;
								new_event->tempID = event->tempID;
								new_event->resend = 0;
								new_event->blk_dir = NULL;
								new_event->cp = event->cp;
								new_event->addr = event->addr;
								new_event->vp = NULL;
								new_event->nbytes = event->cp->bsize;
								new_event->udata = NULL;
								new_event->cmd = Write;
								new_event->rs = event->rs;
								new_event->started = sim_cycle;
								new_event->store_cond = event->store_cond;
								new_event->spec_mode = 0;
								event->childCount++;
								if(!mystricmp(network_type, "FSOI") || (!mystricmp(network_type, "HYBRID")))
								{
#ifdef INV_ACK_CON
									int thread_id;
									if(Is_Shared(LockReg->SubScribeVect, tempID) > 1) 
									{
										if((new_event->src1*mesh_size+new_event->src2 != new_event->des1*mesh_size+new_event->des2) && ((!mystricmp(network_type, "FSOI")) || (!mystricmp(network_type, "HYBRID") && ((abs(new_event->src1 - new_event->des1) + abs(new_event->src2 - new_event->des2)) > 1))))
										{
											new_event->data_reply = 0;
											struct DIRECTORY_EVENT *new_event2 = allocate_event(event->isSyncAccess);
											if(new_event2 == NULL)       panic("Out of Virtual Memory");
											if(collect_stats)
												cp_dir->dir_access ++;
											new_event2->input_buffer_full = 0;
											new_event2->req_time = 0;
											new_event2->conf_bit = -1;
											new_event2->pending_flag = 0;
											new_event2->op = event->op;
											new_event2->isPrefetch = 0;
											new_event2->operation = ACK_MSG_UPDATE;
											new_event2->store_cond = event->store_cond;
											new_event2->src1 = new_event->des1;
											new_event2->src2 = new_event->des2;
											new_event2->des1 = new_event->src1;
											new_event2->des2 = new_event->src2;
											new_event2->processingDone = 0;
											new_event2->startCycle = sim_cycle;
											new_event2->parent = event;
											new_event2->tempID = event->tempID;
											new_event2->resend = 0;
											new_event2->blk_dir = NULL;
											new_event2->cp = event->cp;
											new_event2->addr = event->addr;
											new_event2->vp = NULL;
											new_event2->nbytes = event->cp->bsize;
											new_event2->udata = NULL;
											new_event2->cmd = Write;
											new_event2->rs = event->rs;
											new_event2->started = sim_cycle;
											new_event2->spec_mode = 0;
											new_event2->L2miss_flag = event->L2miss_flag;
											new_event2->data_reply = 0;
											popnetMsgNo ++;
											new_event2->popnetMsgNo = popnetMsgNo;
											new_event2->when = WAIT_TIME;
											dir_eventq_insert(new_event2);
										}
										else	
											new_event->data_reply = 1;
									}
									else	
										new_event->data_reply = 1;
#else
									new_event->data_reply = 1;
#endif
								}
								else 
									new_event->data_reply = 1;
								event->dirty_flag = 1;
								/*go to network*/
								scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, 0);

								dir_eventq_insert(new_event);
							}
						}
						return 1;
					}
				}
				else 
				{ /* miss in Lock Register */
					lock_cache_miss ++;	
					panic("LOCK OPERATION!");
				}
			}
			else /*event is the ACK_MSG_UPDATE*/
			{
				if((event->addr & ~(cache_dl1[0]->bsize - 1)) == Lock_cache[threadid].tag)
				{
					dcache2_access++;
					thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
					(event->parent)->childCount --;

					if((event->parent)->childCount == 0)
					{
						/* Receiving all the invalidation acknowledgement Messages */
						struct DIRECTORY_EVENT *parent_event = event->parent;
						int packet_size = meta_packet_size;

						/* send acknowledgement from directory to requester to write anyway */		    
						parent_event->operation = ACK_DIR_READ_SHARED;
						parent_event->reqAckTime = sim_cycle;
						parent_event->startCycle = sim_cycle;
						parent_event->src1 = event->des1;
						parent_event->src2 = event->des2;
						parent_event->des1 = parent_event->tempID/mesh_size+MEM_LOC_SHIFT;
						parent_event->des2 = parent_event->tempID%mesh_size;
						parent_event->processingDone = 0;
						parent_event->pendingInvAck = 0;
						scheduleThroughNetwork(parent_event, parent_event->startCycle, packet_size, 0);
						free_event(event);
						dir_eventq_insert(parent_event);
						return 1;
					}
					free_event(event);
					return 1;
				}
				else 
				{ /* miss in Lock Register */
					lock_cache_miss ++;	
					panic("LOCK OPERATION!");
				}
			}
		break;
		default:
			/*other load and store access lock data */
			panic("Operations other than LL and SC and not access this function");
	}
}
#endif
extern  short m_shSQSize;
int dir_fifo_enqueue(struct DIRECTORY_EVENT *event, int type)
{
	int threadid = event->des1*mesh_size + event->des2;
	if(event->operation == INV_MSG_READ || event->operation == INV_MSG_WRITE || event->operation == ACK_DIR_IL1 || event->operation == ACK_DIR_READ_SHARED || event->operation == ACK_DIR_READ_EXCLUSIVE || event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE || event->operation == FINAL_INV_ACK || event->operation == NACK || event->operation == FAIL
#ifdef WRITE_EARLY
			|| event->operation == ACK_DIR_WRITE_EARLY
#endif
#ifdef PREFETCH_OPTI
			|| event->operation == READ_META_REPLY_EXCLUSIVE || event->operation == READ_META_REPLY_SHARED || event->operation == READ_DATA_REPLY_EXCLUSIVE || event->operation == READ_DATA_REPLY_SHARED || event->operation == WRITE_META_REPLY || event->operation == WRITE_DATA_REPLY
#endif
	  )
	{
#ifdef LOCK_CACHE_REGISTER
		if(event->operation == INV_MSG_UPDATE)
			if(lock_cache_access(event))
				return 1; /* hit in Lock register, else access L2 cache as normal */
#endif
		if(l1_fifo_num[threadid] >= DIR_FIFO_SIZE)
			panic("L1 cache FIFO is full");
		if(type == 0 && l1_fifo_num[threadid] >= dir_fifo_size)
		{
			if(l1_fifo_num[threadid] >= dir_fifo_size && !last_L1_fifo_full[threadid])
			{
				L1_fifo_full ++;
				last_L1_fifo_full[threadid] = sim_cycle;
			}
			return 0;
		}
		if(last_L1_fifo_full[threadid])
		{
			Stall_L1_fifo += sim_cycle - last_L1_fifo_full[threadid];
			last_L1_fifo_full[threadid] = 0;
		}
		if(event->operation == ACK_DIR_IL1)	event->when = sim_cycle + cache_il1_lat;
		else					event->when = sim_cycle + cache_dl1_lat;
		l1_fifo[threadid][l1_fifo_tail[threadid]] = event;
		l1_fifo_tail[threadid] = (l1_fifo_tail[threadid]+1)%DIR_FIFO_SIZE;
		l1_fifo_num[threadid]++;
		return 1;
	}
	else
	{
#ifdef LOCK_CACHE_REGISTER
		if(event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE || event->operation == ACK_MSG_UPDATE)
			if(lock_cache_access(event))
				return 1; /* hit in Lock register, else access L2 cache as normal */
#endif
#ifdef TSHR
		if(event->flip_flag)
		{
			event->when = sim_cycle+RETRY_TIME;
			tshr_fifo[threadid][tshr_fifo_tail[threadid]] = event;
			tshr_fifo_tail[threadid] = (tshr_fifo_tail[threadid]+1)%TSHR_FIFO_SIZE;
			tshr_fifo_num[threadid]++;
			return 1;
		}
#endif
		if(dir_fifo_num[threadid] >= DIR_FIFO_SIZE)
			panic("DIR FIFO is full");
		if(type == 0 && dir_fifo_num[threadid] >= dir_fifo_size)
		{
			if(dir_fifo_num[threadid] >= dir_fifo_size && !last_Dir_fifo_full[threadid])
			{
				Dir_fifo_full ++;
				last_Dir_fifo_full[threadid] = sim_cycle;
			}
			return 0;
		}
		if(last_Dir_fifo_full[threadid])
		{
			Stall_dir_fifo += sim_cycle - last_Dir_fifo_full[threadid];
			last_Dir_fifo_full[threadid] = 0;
		}
		//event->when = sim_cycle + cache_dl2_lat;
		if(event->operation != MEM_READ_REQ && event->operation != MEM_READ_REPLY && event->operation != WAIT_MEM_READ_NBLK)
			event->when = sim_cycle + cache_dl2_lat;
		else
			event->when = sim_cycle;
		dir_fifo[threadid][dir_fifo_tail[threadid]] = event;
		dir_fifo_tail[threadid] = (dir_fifo_tail[threadid]+1)%DIR_FIFO_SIZE;
		dir_fifo_num[threadid]++;
		return 1;
	}
}

void dir_fifo_dequeue()
{
	int i, j;
#ifdef LOCK_CACHE_REGISTER
	int entry;
	for(i=0;i<numcontexts;i++)
	{
		if(lock_fifo_num[i])
		{
			for(j=0;j<MAXLOCKEVENT;j++)
			{
				if(lock_cache_operation(lock_fifo[i][lock_fifo_head[i]])) /* 1 means it's a lock cache access */
				{
					lock_fifo_num[i] --;
					lock_fifo_benefit ++;
					lock_fifo_head[i] = (lock_fifo_head[i]+1+MAXLOCKEVENT)%MAXLOCKEVENT;
				}
				else
					break;
				if(lock_fifo_num[i] == 0)
					break;
			}
		}
	}
#endif
#ifdef TSHR
	/* L2 and dir queue */
	for(i = 0; i < numcontexts+mesh_size*2; i++)
	{
		if(tshr_fifo_num[i] <= 0 || tshr_fifo_portuse[i] >= DIR_FIFO_PORTS)
			continue;
		while(1)
		{
			if(tshr_fifo[i][tshr_fifo_head[i]]->when > sim_cycle)
				break;
			if(dir_operation(tshr_fifo[i][tshr_fifo_head[i]], 0) == 0) /* 0 means it's a normal cache access */
				break;
			tshr_fifo_portuse[i]++;
			tshr_fifo_head[i] = (tshr_fifo_head[i]+1)%TSHR_FIFO_SIZE;
			tshr_fifo_num[i]--;
			if(tshr_fifo_portuse[i] >= DIR_FIFO_PORTS || tshr_fifo_num[i] <= 0)
				break;		
		}
	}
#endif
		
	/* L2 and dir queue */
	for(i = 0; i < numcontexts+mesh_size*2; i++)
	{
		// deleted by wxh
		if(dir_fifo_num[i] <= 0 /*|| dir_fifo_portuse[i] >= DIR_FIFO_PORTS*/ )
			continue;
		while(1)
		{
		        struct DIRECTORY_EVENT* event = dir_fifo[i][dir_fifo_head[i]];
   		        if(dir_fifo[i][dir_fifo_head[i]]->when > sim_cycle)
			  {
			    int wxh =2;
			    wxh ++;
			    break;
			  }
			if(dir_operation(dir_fifo[i][dir_fifo_head[i]], 0) == 0) /* 0 means it's a normal cache access */
			  {
			    int wxh = 0;
			    wxh ++;
			    break;
			  }
			//printf ("wxh event deQed into dir, PC %x, operation %d, src (%d,%d), dest (%d, %d), cmd %d, # %d, access addr %x \n", event->rs?event->rs->PC:0, event->operation, event->src1, event->src2, event->des1, event->des2, event->cmd, event->popnetMsgNo, event->addr);
			dir_fifo_portuse[i]++;
			dir_fifo_head[i] = (dir_fifo_head[i]+1)%DIR_FIFO_SIZE;
			dir_fifo_num[i]--;
			// deleted by wxh
			if(/*dir_fifo_portuse[i] >= DIR_FIFO_PORTS ||*/ dir_fifo_num[i] <= 0)
				break;		
		}
	}

	/* L1 queues */
	for(i = 0; i < numcontexts+mesh_size*2; i++)
	{
		extern struct res_pool *fu_pool;

		if(l1_fifo_num[i] <= 0 || l1_fifo_portuse[i] >= DIR_FIFO_PORTS)
			continue;
		while(1)
		{
			if(l1_fifo[i][l1_fifo_head[i]]->when > sim_cycle)
				break;
			if(dir_operation(l1_fifo[i][l1_fifo_head[i]], 0) == 0)
				break;
			l1_fifo_portuse[i]++;
			l1_fifo_head[i] = (l1_fifo_head[i]+1)%DIR_FIFO_SIZE;
			l1_fifo_num[i]--;
			if(l1_fifo_portuse[i] >= DIR_FIFO_PORTS || l1_fifo_num[i] <= 0)
				break;		
		}
	}
}
/*		DIR FIFO IMPLEMENTATION ENDS		*/



void dir_eventq_insert(struct DIRECTORY_EVENT *event)     // event queue directory version, put dir event in queue
{
	struct DIRECTORY_EVENT *ev, *prev;     


	for (prev = NULL, ev = dir_event_queue; ev && ((ev->when) <= (event->when)); prev = ev, ev = ev->next);
	if (prev)
	{
		event->next = ev;
		prev->next = event;
	}
	else
	{
		event->next = dir_event_queue;
		dir_event_queue = event;
	}
}

#ifdef CONF_RES_RESEND
/* this queue is the sending event queue at the request and reply event queue at the home node */
void queue_insert(struct DIRECTORY_EVENT *event, struct QUEUE_EVENT *queue)
{ /* queue_insert */
	int i = 0; 
	if(queue->free_Entries == 0)
		panic("There is no free entry to insert in sending message queue");

	for(i=0; i<QUEUE_SIZE; i++)
	{
		if(!queue->Queue_entry[i].isvalid)
		{
			queue->Queue_entry[i].isvalid = 1;
			queue->Queue_entry[i].event = event;
			queue->free_Entries --;
			return;
		}
	}

}

void free_queue(struct DIRECTORY_EVENT *event, struct QUEUE_EVENT *queue)
{
	int i = 0;
	if(!queue) return;
	for(i=0; i<QUEUE_SIZE; i++)
	{
		if(queue->Queue_entry[i].isvalid && (queue->Queue_entry[i].event->addr == event->addr) && (queue->Queue_entry[i].event->started == event->started) 
				&& (queue->Queue_entry[i].event->missNo == event->missNo))
		{
			queue->Queue_entry[i].isvalid = 0;
			queue->free_Entries ++;
			break;
		}
	}

	if(queue->free_Entries > QUEUE_SIZE)
		panic("QUEUE SIZE EXCEEDS\n");

}

int queue_check(struct DIRECTORY_EVENT *event, struct QUEUE_EVENT *queue)
{
	int i = 0;
	if(!queue) return 0;
	for(i=0; i<QUEUE_SIZE; i++)
	{
		if(queue->Queue_entry[i].isvalid && (queue->Queue_entry[i].event->addr == event->addr) && (queue->Queue_entry[i].event->started == event->started)
				&& (queue->Queue_entry[i].event->missNo == event->missNo))
			return 1;
	}
	return 0;
}
#endif

void eventq_update(struct RUU_station *rs, tick_t ready)   /*directory event queue version */
{
	struct RS_link *ev, *prev, *new_ev=NULL;

	if(rs == NULL || event_queue == NULL)
	{
		return;
	}

	for (prev=NULL, ev=event_queue; (ev->next!=NULL); prev=ev, ev=ev->next)
	{
		if((ev->rs == rs) && (ev->tag == rs->tag))
		{
			break;
		}
	}

	if ((ev->rs == rs) && (ev->tag == rs->tag))                     //mwr: FIXED = to ==
	{
		if (prev)
		{
			prev->next = ev->next;
		}
		else
		{
			event_queue = ev->next;
		}
		new_ev = ev;
		new_ev->x.when = ready;

		for (prev=NULL, ev=event_queue; ev && ev->x.when < ready; prev=ev, ev=ev->next);
		if (prev)
		{
			new_ev->next = prev->next;
			prev->next = new_ev;
		}
		else
		{
			new_ev->next = event_queue;
			event_queue = new_ev;
		}
	}
}
#ifdef TSHRn
void initTSHR(struct tshr_t *tshr, int flag)
{
	int i;
	tshr->tshrSIZE = TSHR_SIZE;
	tshr->freeEntries = TSHR_SIZE;
	for (i = 0; i < TSHR_SIZE; i++)
	{
		tshr->tshrEntry[i].id = i;
		tshr->tshrEntry[i].isValid = 0;
		tshr->tshrEntry[i].event = NULL;
		tshr->tshrEntry[i].addr = 0;
	}
}
void TSHRInsert(struct tshr_t *tshr, struct DIRECTORY_EVENT *event, md_addr_t addr)
{
	int i;

	if(tshr->freeEntries == 0)
		panic("There is no free entry to insert in TSHR");

	for (i = 0; i < TSHR_SIZE; i++)
	{
		if(!tshr->mshrEntry[i].isValid)
		{
			tshr->tshrEntry[i].isValid = 1; 
			tshr->tshrEntry[i].event = event;
			tshr->tshrEntry[i].pending_addr = (addr & ~cache_dl2->bsize);
			tshr->freeEntries--;
			return;
		}
	}
	panic("There is no free entry to insert in MSHR");
}
void TSHRWakeup(struct tshr_t *tshr, int wakeup_addr)
{
	int i;

	for (i = 0; i < TSHR_SIZE; i++)
	{
		if(tshr->mshrEntry[i].isValid && (tshr->tshrEntry[i].pending_addr == (wakeup_addr & ~cache_dl2->bsize)))
		{
			if(dir_operation(tshr->tshrEntry[i].event))
			{
				tshr->tshrEntry[i].isValid = 1; 
				tshr->tshrEntry[i].event = NULL;
				tshr->tshrEntry[i].pending_addr = 0;
				tshr->freeEntries++;
			}
			else
				panic("can not wakeup becuase the blk is still in transient state");
			return;
		}
	}
	panic("There is no match entry in TSHR");
		
}
#endif

#ifdef DCACHE_MSHR
//struct DIRECTORY_EVENT *event_created = NULL;
void initMSHR(struct mshr_t *mshr, int flag)
{
	int i, mshr_size;
	if(!mshr) return;
	if(flag == 1)
		mshr_size = L2_MSHR_SIZE;
	else
		mshr_size = MSHR_SIZE;	

	mshr->mshrSIZE = mshr_size;
	mshr->freeEntries = mshr_size;
#ifdef 	EDA
	mshr->freeOCEntries = MSHR_OC_SIZE;
#endif
	for (i = 0; i < mshr_size; i++)
	{
		mshr->mshrEntry[i].id = i;
		mshr->mshrEntry[i].isValid = 0;
		mshr->mshrEntry[i].addr = 0;
		mshr->mshrEntry[i].startCycle = 0;
		mshr->mshrEntry[i].endCycle = 0;
		mshr->mshrEntry[i].blocknum = 0;
#ifdef	EDA
		mshr->mshrEntry[i].isOC = 0;
#endif
	}
}

int isMSHRFull(struct mshr_t *mshr, int isOC, int id)
{
	if(!mshr) return 0;
	if(mshr->freeEntries == 0 && !last_L1_mshr_full[id])
	{
		L1_mshr_full ++;
		last_L1_mshr_full[id] = sim_cycle;
	}
	else
	{
		if(last_L1_mshr_full[id])
		{
			Stall_L1_mshr += sim_cycle - last_L1_mshr_full[id];
			last_L1_mshr_full[id] = 0;
		}
	}
	return (mshr->freeEntries == 0);
}

void freeMSHR(struct mshr_t *mshr)
{
	int i;
	if(!mshr) return;
	for (i = 0; i < MSHR_SIZE; i++)
	{
		if(mshr->mshrEntry[i].isValid && mshr->mshrEntry[i].endCycle <= sim_cycle)
		{
			mshr->mshrEntry[i].isValid = 0;
			mshr->freeEntries++;
			struct RS_list *prev, *cur;
			cur = rs_cache_list[mshr->threadid][i];
			while(cur)
			{
				if(cur->rs && ((cur->rs->addr)>>cache_dl1[mshr->threadid]->set_shift) != (mshr->mshrEntry[i].addr>>cache_dl1[mshr->threadid]->set_shift))
					panic("L1 MSHR: illegal address in MSHR list");

				prev = cur->next;
				RS_block_next(cur);
				free(cur);
				cur = prev;
			} 
			rs_cache_list[mshr->threadid][i] = NULL;
		}
	}
	if (mshr->freeEntries > MSHR_SIZE)
		panic("MSHR SIZE exceeds\n");
}

void freeL2MSHR(struct mshr_t *mshr)
{
	int i;
	if(!mshr) return;
	for (i = 0; i < L2_MSHR_SIZE; i++)
	{
		if(mshr->mshrEntry[i].isValid && mshr->mshrEntry[i].endCycle <= sim_cycle)
		{
			mshr->mshrEntry[i].isValid = 0;
			mshr->mshrEntry[i].blocknum = 0;
			mshr->freeEntries++;
		}
	}
	if (mshr->freeEntries > L2_MSHR_SIZE)
		panic("MSHR SIZE exceeds\n");
}

int MSHR_block_check(struct mshr_t *mshr, md_addr_t addr, int offset)
{
	int i, match = 0, mshr_size;
	mshr_size = mshr->mshrSIZE;

	for (i=0; i<mshr_size; i++)
	{
		if(mshr->mshrEntry[i].isValid)
		{
			if(((mshr->mshrEntry[i].addr>>offset) == (addr>>offset)))
			{
				if(match != 0) 	panic("MSHR: a miss address belongs to more than two MSHR Entries");
				match = (i+1);
			}
		}
	}
	return match;
}

/* per cache line MSHR lookup. Insert if there is hit */
void MSHRLookup(struct mshr_t *mshr, md_addr_t addr, int lat, int isOC, struct RUU_station *rs)
{
	int i, mshr_size;
	if(!mshr) return;

	if(lat < cache_dl1_lat)
		return;

	mshr_size = mshr->mshrSIZE;

	if(rs && rs->threadid != mshr->threadid)
		panic("L1 MSHR wrong threadid");

	if(mshr->freeEntries == 0)
		panic("There is no free entry to insert in MSHR");

	for (i = 0; i < mshr_size; i++)
	{
		if(!mshr->mshrEntry[i].isValid)
		{
			mshr->mshrEntry[i].isValid = 1; 
			mshr->mshrEntry[i].addr = addr;
			mshr->mshrEntry[i].startCycle =  sim_cycle;
			mshr->mshrEntry[i].endCycle = sim_cycle + lat;
			mshr->mshrEntry[i].event = event_created;
			mshr->mshrEntry[i].rs = rs;
			mshr->freeEntries--;
			return;
		}
	}

	panic("There is no free entry to insert in MSHR");
}
void mshrentry_update(struct DIRECTORY_EVENT *event, tick_t ready, tick_t startcycle)
{
	int i, mshr_size;
	int threadid = event->tempID;
	if(!cache_dl1[threadid]->mshr)
		return;
	mshr_size = cache_dl1[threadid]->mshr->mshrSIZE;
	for (i=0;i<mshr_size;i++)
	{
		if(cache_dl1[threadid]->mshr->mshrEntry[i].isValid 
				&& ((cache_dl1[threadid]->mshr->mshrEntry[i].addr >> cache_dl1[threadid]->set_shift) == (event->addr >> cache_dl1[threadid]->set_shift))) 
		{
#ifndef EUP_NETWORK
			if(cache_dl1[threadid]->mshr->mshrEntry[i].startCycle != startcycle)
				panic("L1 MSHR: illegal match in mshr");
#endif
			cache_dl1[threadid]->mshr->mshrEntry[i].endCycle = ready;
			break;
		}
	}
}
#ifdef READ_PERMIT
counter_t mshr_check_rs(int threadid, md_addr_t addr, struct RUU_station **rs)
{
	int i;
	if(!cache_dl1[threadid]->mshr)
		return;
	for(i=0;i<MSHR_SIZE;i++)
	{
		if(cache_dl1[threadid]->mshr->mshrEntry[i].isValid 
				&& ((cache_dl1[threadid]->mshr->mshrEntry[i].addr >> cache_dl1[threadid]->set_shift) == (addr >> cache_dl1[threadid]->set_shift))) 
		{
			*rs = cache_dl1[threadid]->mshr->mshrEntry[i].rs;
			return cache_dl1[threadid]->mshr->mshrEntry[i].startCycle;
		}	
	}
}
#endif
#endif

int local_access_check(long src1, long src2, long des1, long des2)
{
	int flag;
	if((src1*mesh_size+src2)==(des1*mesh_size+des2))
		flag = 1;
	else
		flag = 0;
	return flag;
}
int findCacheStatus(struct cache_t *cp, md_addr_t set, md_addr_t tag, int hindex, struct cache_blk_t **blk)
{
	struct cache_blk_t *findblk;

	for (findblk=cp->sets[set].way_head; findblk; findblk=findblk->way_next)
	{
		if((findblk->tagid.tag == tag)  && (findblk->status & CACHE_BLK_VALID)
#if PROCESS_MODEL
				&& (isSharedAddress == 1)
#endif
		  )
		{
			*blk = findblk;
			return 1;
		}
	}
	return 0;
}
int Is_Shared(unsigned long long int *sharer, int threadid)
{
	int count;
	int counter = 0;
	for (count = 0; count < numcontexts; count++)
	{
		if((((sharer[count/64]) & ((unsigned long long int)1 << (count%64))) == ((unsigned long long int)1 << (count%64)))) // && (count != threadid))
		{
			counter ++;
		}
	}
	if(counter)
		return counter;
	else
		return 0;

}
int Is_ExclusiveOrDirty(unsigned long long int *sharer, int threadid, int *ExclusiveID)
{
	int count, Exclusive_id;
	int counter = 0;
	for (count = 0; count < numcontexts; count++)
	{
		if((((sharer[count/64]) & ((unsigned long long int)1 << (count%64))) == ((unsigned long long int)1 << (count%64))))// && (count != threadid))
		{
			counter ++;	
			Exclusive_id = count;
		}
	}
	if(counter == 1)
	{
		(*ExclusiveID) = Exclusive_id; 
		return 1;
	}
	else
		return 0; 
}

int IsShared(unsigned long long int *sharer, int threadid)
{
	int count;
	int counter = 0;
	for (count = 0; count < numcontexts; count++)
	{
		if((((sharer[count/64]) & ((unsigned long long int)1 << (count%64))) == ((unsigned long long int)1 << (count%64))) && (count != threadid))
		{
			counter ++;
		}
	}
	if(counter)
		return 1;
	else
		return 0;

}
int IsExclusiveOrDirty(unsigned long long int *sharer, int threadid, int *ExclusiveID)
{
	int count, Exclusive_id;
	int counter = 0;
	for (count = 0; count < numcontexts; count++)
	{
		if((((sharer[count/64]) & ((unsigned long long int)1 << (count%64))) == ((unsigned long long int)1 << (count%64))) && (count != threadid))
		{
			counter ++;	
			Exclusive_id = count;
		}
	}
	if(counter == 1)
	{
		(*ExclusiveID) = Exclusive_id; 
		return 1;
	}
	else
		return 0; 
}

/* L2 FIFO queue*/



/* insert event into event_list appending to a mshr entry */ /* jing */
void event_list_insert(struct DIRECTORY_EVENT *event, int entry)
{
	if(entry < 1 || entry > L2_MSHR_SIZE) 	panic("L2 MSHR insertion error");
	entry = entry - 1;
	struct DIRECTORY_EVENT *cur, *prev;

	if(event_list[entry] && ((event->addr >> cache_dl2->set_shift) != (event_list[entry]->addr >> cache_dl2->set_shift)))
		panic("L2 MSHR insertion error");
	//event->next = event_list[entry];
	//event_list[entry] = event;
	if(sim_cycle == cache_dl2->mshr->mshrEntry[entry].endCycle)
		dir_eventq_insert(event);
	else
	{
		event->next = event_list[entry];
		event_list[entry] = event;
	}
}
/* update L2 MSHR entry when a missing is handled over*/ /* jing */
void update_L2_mshrentry(md_addr_t addr, tick_t ready, tick_t startcycle)
{
	int i;
	if(!cache_dl2->mshr)
		panic("L2 CACHE did not have MSHR!");
	struct DIRECTORY_EVENT *cur, *prev;
	for (i=0;i<L2_MSHR_SIZE;i++)
	{
		if(cache_dl2->mshr->mshrEntry[i].isValid
				&& (cache_dl2->mshr->mshrEntry[i].addr == addr))
		{  /* Find the entry which should be updated*/
			if(cache_dl2->mshr->mshrEntry[i].startCycle != startcycle)
				panic("L2 MSHR: illegal match in MSHR while update");
			cache_dl2->mshr->mshrEntry[i].blocknum ++;
			if(cache_dl2->mshr->mshrEntry[i].blocknum < 2)
				return;	
			cache_dl2->mshr->mshrEntry[i].endCycle = ready;
			cache_dl2->mshr->mshrEntry[i].blocknum = 0;
			for(cur = event_list[i]; cur; cur=prev)
			{
				if(((cur->addr)>>cache_dl2->set_shift) != ((cache_dl2->mshr->mshrEntry[i].addr>>cache_dl2->set_shift)))
				{
					printf("cur addr %lld, mshr addr %lld\n", cur->addr, cache_dl2->mshr->mshrEntry[i].addr);
					panic("Eventlist should have the same address with its appending mshr entry!");
				}
				prev = cur->next;
				cur->next = NULL;
				cur->when = sim_cycle;
				dir_eventq_insert(cur);
			}
			event_list[i] = NULL;
			break;
		}
	}
}

/* check the sharer is in the neighborhood of threadid */
int IsNeighbor(unsigned long long int sharer, int threadid, int type, md_addr_t addr)
{
	int count, close, far, data_close, data_far, data_corner, total_sharer, total_data_sharer, corner, data_flag = 0;
	struct cache_t *cp1;
	struct cache_blk_t *blk1;
	md_addr_t tag, set;		//owner L1 cache tag and set
	close = 0;
	corner = 0;
	far = 0;
	total_sharer = 0;
	total_data_sharer = 0;
	total_p_c_events ++;

	for (count = 0; count < numcontexts; count++)
	{
		if((((sharer) & ((unsigned long long int)1 << count)) == ((unsigned long long int)1 << count)) && (count != threadid))
		{
			cp1 = cache_dl1[count];
			tag = CACHE_TAG(cp1,addr);
			set = CACHE_SET(cp1,addr);
			for (blk1 = cp1->sets[set].way_head; blk1; blk1 =blk1->way_next)
			{
				if ((blk1->tagid.tag == tag) && (blk1->status & CACHE_BLK_VALID))
				{
					if(blk1->state == MESI_MODIFIED)
					{
						data_flag = 1;
						total_data_consumers ++;
						total_data_sharer ++;
					}
				}
			}

			total_consumers ++;
			total_sharer ++;

			if(abs(threadid - count) == 3 || abs(threadid - count) == 5)
			{
				total_packets_at_corners ++;
				corner ++;
				if(data_flag == 1)
					total_data_at_corner ++;
			}

			if(abs(threadid - count) == 1 || abs(threadid - count) == 4)	
			{
				close ++;
				total_packets_in_neighbor ++;
				if(data_flag == 1)
					total_data_close ++;
			}
			else
			{
				far ++;
				if(data_flag == 1)
					total_data_far ++;
			}
		}
	}
	//	if(total_data_sharer > 1)
	//		panic("Neighborhood Check: can not have two modified suppliser!");
	if((corner + close) == total_sharer)
		total_all_almostclose ++;

	if(close == total_sharer)
		total_all_close ++;	
	else
	{
		total_not_all_close ++;		
		average_inside_percent += ((double)close/(double)total_sharer);
		average_outside_percent += ((double)far/(double)total_sharer);
		average_outside_abs_percent += ((double)(far-corner)/(double)total_sharer);
		average_corner_percent += ((double)(corner)/(double)total_sharer);
	}

	switch (total_sharer)
	{
	//	case 0: panic("total_sharers can not be 0!");
		case 1: sharer_num[1] ++; break;
		case 2: sharer_num[2] ++; break;
		case 3: sharer_num[3] ++; break;
		case 4: sharer_num[4] ++; break;
		case 5: sharer_num[5] ++; break;
		case 6: sharer_num[6] ++; break;
		case 7: sharer_num[7] ++; break;
		case 8: sharer_num[8] ++; break;
		case 9: sharer_num[9] ++; break;
		case 10: sharer_num[10] ++; break;
		case 11: sharer_num[11] ++; break;
		case 12: sharer_num[12] ++; break;
		case 13: sharer_num[13] ++; break;
		case 14: sharer_num[14] ++; break;
			 //case 15: sharer_num[15] ++; break;
		default: sharer_num[15] ++; break;
	}

}
int IsNeighbor_sharer(unsigned long long int sharer, int threadid)
{
	int count;

	for (count = 0; count < numcontexts; count++)
	{
		if((((sharer) & ((unsigned long long int)1 << count)) == ((unsigned long long int)1 << count)) && (count != threadid))
		{
			if(abs(threadid - count) == 3 || abs(threadid - count) == 5)
				total_corner_packets ++;

			if(abs(threadid - count) == 1 || abs(threadid - count) == 4)	
				total_neighbor_packets ++;
			else
				total_far_packets ++;
		}
	}
}

int blockoffset(md_addr_t addr)
{
	int bofs;
	bofs = addr & cache_dl2->blk_mask;
	bofs = bofs >> cache_dl1[0]->set_shift;
	return bofs;
}

struct DIRECTORY_EVENT *allocate_new_event(struct DIRECTORY_EVENT *event)
{
	struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
	if(new_event == NULL)       panic("Out of Virtual Memory");

	new_event->input_buffer_full = 0;
	new_event->req_time = 0;
	new_event->conf_bit = -1;
	new_event->pending_flag = 0;
	new_event->isPrefetch = 0;
	new_event->processingDone = 0;
	new_event->startCycle = sim_cycle;
	new_event->started = sim_cycle;
	new_event->blk_dir = NULL;
	new_event->cp = event->cp;
	new_event->addr = event->addr;
	new_event->vp = NULL;
	new_event->nbytes = event->cp->bsize;
	new_event->udata = NULL;
	new_event->rs = event->rs;
	new_event->spec_mode = 0;
	new_event->L2miss_flag = event->L2miss_flag;
	new_event->data_reply = 1;
	new_event->mshr_time = event->mshr_time;
	new_event->store_cond = event->store_cond;
	new_event->originalChildCount = 0;
#ifdef SMD_USE_WRITE_BUF
	new_event->l1_wb_entry = NULL;
#endif

	new_event->op = event->op;

	new_event->src1 = event->des1;
	new_event->src2 = event->des2;
	if((event->des1-MEM_LOC_SHIFT)/(mesh_size/2))
		new_event->des1 = mesh_size+1;   /* each column has a memory access port */
	else
		new_event->des1 = 0;

	//new_event->des2 = ((event->des2)/(mesh_size/(mem_port_num/2))) * (mesh_size/(mem_port_num/2));	     /* the memory access port allocate in the middle of those routers. */ 
	switch(event->des2/(mesh_size/(mem_port_num/2))) /* right side memory port */
	{
		case 0:
			new_event->des2 = mesh_size/(mem_port_num/2) - 1; 
		break;
		case 1:
			new_event->des2 = mesh_size/(mem_port_num/2); 
		break;
		case 2:
			new_event->des2 = 3*(mesh_size/(mem_port_num/2)) - 1; 
		break;
		case 3:
			new_event->des2 = 3*(mesh_size/(mem_port_num/2)); 
		break;
		case 4:
			new_event->des2 = 5*(mesh_size/(mem_port_num/2)) - 1; 
		break;
		case 5:
			new_event->des2 = 5*(mesh_size/(mem_port_num/2)); 
		break;
		case 6:
			new_event->des2 = 7*(mesh_size/(mem_port_num/2)) - 1; 
		break;
		case 7:
			new_event->des2 = 7*(mesh_size/(mem_port_num/2)); 
		break;
		default:
			panic("can not have other case\n");
	}

	new_event->parent = event;
	new_event->tempID = event->tempID;
	new_event->resend = 0;
	return new_event;
}
#ifdef POPNET
struct OrderBufferEntry
{
	int valid;
	md_addr_t addr;
	int des;
	int vc;
	counter_t msgno;
};
struct OrderBufferEntry OrderBuffer[MAXSIZE][MAXSIZE];
int OrderBufferNum[MAXSIZE];

void OrderBufInit()
{
	int i=0, j=0;
	for(i=0;i<MAXSIZE;i++)
	{
		OrderBufferNum[i] = 0;	
		for(j=0;j<MAXSIZE;j++)
		{
			OrderBuffer[i][j].addr = 0;
			OrderBuffer[i][j].valid = 0;
			OrderBuffer[i][j].des = 0;
			OrderBuffer[i][j].vc = -1;
			OrderBuffer[i][j].msgno = 0;
		}
	}
}
void OrderBufInsert(int s1, int s2, int d1, int d2, md_addr_t addr, int vc, counter_t msgno)
{
	int i=0, src, des;
	src = s1*mesh_size+s2;
	des = d1*mesh_size+d2;

	if(OrderBufferNum[src] >= MAXSIZE)
		panic("Out going buffer is full for popnet multiple vcs");

	for(i=0;i<MAXSIZE;i++)
	{
		if(!OrderBuffer[src][i].valid)
		{
			OrderBuffer[src][i].valid = 1;
			OrderBuffer[src][i].addr = addr;
			OrderBuffer[src][i].des = des;
			OrderBuffer[src][i].vc = vc;
			OrderBuffer[src][i].msgno = msgno;
			OrderBufferNum[src] ++;
			return;
		}
	}
}
void OrderBufRemove(int s1, int s2, int d1, int d2, md_addr_t addr, counter_t msgno)
{
	int i=0, src, des;
	src = s1*mesh_size+s2;
	des = d1*mesh_size+d2;

	for(i=0;i<MAXSIZE;i++)
	{
		if(OrderBuffer[src][i].valid && OrderBuffer[src][i].addr == addr && OrderBuffer[src][i].des == des && OrderBuffer[src][i].msgno == msgno)	
		{
			OrderBuffer[src][i].valid = 0;
			OrderBuffer[src][i].addr = 0;
			OrderBuffer[src][i].des = 0;
			OrderBuffer[src][i].vc = -1;
			OrderBuffer[src][i].msgno = 0;
			OrderBufferNum[src] --;
			return;
		}
	}
	panic("Can not find the entry in OrderBuffer");
}

int OrderCheck(int s1, int s2, int d1, int d2, md_addr_t addr)
{
	int i=0, src, des;
	src = s1*mesh_size+s2;
	des = d1*mesh_size+d2;
#ifdef MULTI_VC
	for(i=0;i<MAXSIZE;i++)
	{
		if(OrderBuffer[src][i].valid && OrderBuffer[src][i].addr == addr && OrderBuffer[src][i].des == des)	
			return OrderBuffer[src][i].vc;
	}
#endif
	return -1;
}
#endif

void timespace(counter_t time, int type)
{
	if(time < 0)
		return;
	if(time <5)
		spand[0] ++;
	else if(time <10)
		spand[1] ++;
	else if(time <20)
		spand[2] ++;
	else if(time <30)
		spand[3] ++;
	else 
		spand[4] ++;
	if(type == 1)
		downgrade_r ++;	
	else 
		downgrade_w ++;
	return;
}
int dir_operation (struct DIRECTORY_EVENT *event, int lock_access)
{
	extern counter_t icache_access;
	extern counter_t dcache_access;

	int lat;
	md_addr_t tag_dir, set_dir;
	md_addr_t tag_prefetch, set_prefetch, addr_prefetch; //L2 cache tag and set for prefetch
	md_addr_t tag, set;		//owner L1 cache tag and set
	int hindex_dir;
	struct cache_blk_t *blk;	//block in cache L2 if there is a L2 hit
	struct cache_blk_t *blk1;	//block in cache L1
	struct cache_blk_t *repl;	//replacement block in cache L1

	int matchnum; 			//match entry index in L2 mshr
	int Threadid;
	int tempID =  event->tempID;	//requester thread id
	int count; 			//owner thread id
	struct cache_t *cp_dir = cache_dl2, *cp1;//cp1 is the owner L1 cache
	md_addr_t addr=event->addr;
	unsigned int packet_size; 
	tag_dir = CACHE_TAG(cp_dir, addr);	//L2 cache block tag
	set_dir = CACHE_SET(cp_dir, addr);	//L2 cache block set
	hindex_dir = (int) CACHE_HASH(cp_dir,tag_dir);
	md_addr_t bofs;
	int block_offset = blockoffset(event->addr);
	int invCollectStatus = 0;
	int vc = 0, a = 0;
	//if(set_dir == 921 && tag_dir == 40960 && block_offset == 0)
	//	printf("DEBUG:threadid %d operate %d at sim_cycle %llu\n", tempID, event->operation, sim_cycle);
	int options = -1;
	event->flip_flag = 0;

	if(!(event->operation == ACK_DIR_IL1 || event->operation == ACK_DIR_READ_SHARED || event->operation == ACK_DIR_READ_EXCLUSIVE || event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE
#ifdef PREFETCH_OPTI
		|| event->operation == READ_META_REPLY_EXCLUSIVE || event->operation == READ_META_REPLY_SHARED || event->operation == READ_DATA_REPLY_EXCLUSIVE || event->operation == READ_DATA_REPLY_SHARED || event->operation == WRITE_META_REPLY || event->operation == WRITE_DATA_REPLY
#endif
	))
	{
		/* check input buffer space for the scheduling event */
		int buffer_full_flag = 0;
		if(!mystricmp (network_type, "FSOI") || !(mystricmp (network_type, "HYBRID")))
			buffer_full_flag = opticalBufferSpace(event->des1, event->des2, event->operation);
		else if(!mystricmp (network_type, "MESH"))
		{
			if(!(event->operation == ACK_DIR_IL1 || event->operation == ACK_DIR_READ_SHARED || event->operation == ACK_DIR_READ_EXCLUSIVE || event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE || event->operation == FINAL_INV_ACK || event->operation == MEM_READ_REQ || event->operation == MEM_READ_REPLY || event->operation == WAIT_MEM_READ_NBLK || (event->des1 == event->src1 && event->des2 == event->src2)
#ifdef PREFETCH_OPTI
		|| event->operation == READ_META_REPLY_EXCLUSIVE || event->operation == READ_META_REPLY_SHARED || event->operation == READ_DATA_REPLY_EXCLUSIVE || event->operation == READ_DATA_REPLY_SHARED || event->operation == WRITE_META_REPLY || event->operation == WRITE_DATA_REPLY
#endif
		))
			{
				options = OrderCheck(event->des1, event->des2, event->src1, event->src2, addr&~(cache_dl1[0]->bsize-1));
				vc = popnetBufferSpace(event->des1, event->des2, options);
			}
			buffer_full_flag = (vc == -1);
		}
		else if(!mystricmp (network_type, "COMB"))
		{
			buffer_full_flag = CombNetworkBufferSpace(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1), event->operation, &vc);
		}
		if(buffer_full_flag)
		{
			if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
			{
				Input_queue_full ++;
				last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
			}
			return 0;
		}
		else
		{
			if(last_Input_queue_full[event->des1*mesh_size+event->des2])
			{
				Stall_input_queue += sim_cycle - last_Input_queue_full[event->des1*mesh_size+event->des2];
				last_Input_queue_full[event->des1*mesh_size+event->des2] = 0;	
			}
		}
	}

	switch(event->operation)
	{
		case IDEAL_CASE		: 
			panic("DIR: Event operation is ideal - not possible");
			break;
#ifdef REAL_LOCK
		case FAIL:
			if(!event->spec_mode && event->rs)
			{
					event->rs->writewait = 2;
					event->rs->STL_C_fail = 1;
			}
#ifdef SMD_USE_WRITE_BUF
			else if(event->l1_wb_entry)
				event->l1_wb_entry->STL_C_fail = 1;
#endif
			
			mshrentry_update(event, sim_cycle, event->started);

			free_event(event);
			
			return 1;
		break;
#endif
		case NACK		:
			/* this is the nack sending from L2 because of the dir_fifo full */
			nack_counter ++;
			event->src1 += event->des1;
			event->des1 = event->src1 - event->des1;
			event->src1 = event->src1 - event->des1;
			event->src2 += event->des2;
			event->des2 = event->src2 - event->des2;
			event->src2 = event->src2 - event->des2;
			event->operation = event->parent_operation;
			event->processingDone = 0;
			event->startCycle = sim_cycle + 20;
			scheduleThroughNetwork(event, event->startCycle, meta_packet_size, vc);
			if(event->prefetch_next == 2 || event->prefetch_next == 4)
				prefetch_nacks ++;
			else if(event->isSyncAccess)
				sync_nacks ++;
			else if(event->operation == MISS_WRITE)
				write_nacks ++;
			else
				normal_nacks ++;
			//printf("event is prefetch or not %d\n", event->prefetch_next);
			dir_eventq_insert(event);
			if(collect_stats)
				cp_dir->dir_access ++;
			return 1;

		case MEM_READ_REQ 	:
			lat = cp_dir->blk_access_fn(Read, CACHE_BADDR(cp_dir,addr), cp_dir->bsize, NULL, sim_cycle, tempID);
			event->when = sim_cycle + lat;
			event->startCycle = sim_cycle;
			event->operation = MEM_READ_REPLY;
			total_mem_lat += lat;
			total_mem_access ++;
			dir_eventq_insert(event);
			break;

		case MEM_READ_REPLY	:
			{
				int i=0;
				for(i=0;i<(cache_dl2->bsize/max_packet_size);i++)
				{
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->src1 = event->des1;
					new_event->src2 = event->des2;
					new_event->des1 = event->src1;
					new_event->des2 = event->src2;
					new_event->processingDone = 0;
					new_event->operation = WAIT_MEM_READ;
					new_event->req_time = event->req_time;
					event->operation = WAIT_MEM_READ;
					new_event->startCycle = sim_cycle;
					event->childCount ++;
					if(i==0)
					{ /* serve the demanding block first */
						scheduleThroughNetwork(new_event, sim_cycle, data_packet_size, vc);
						dir_eventq_insert(new_event);
					}
					else
					{
						new_event->operation = WAIT_MEM_READ_NBLK;
						new_event->when = sim_cycle + ((cache_dl1[0]->bsize*mem_port_num/mem_bus_width)*((float)4/mem_bus_speed))*i;
						dir_eventq_insert(new_event);
					}
				}
			}
			break;
		case WAIT_MEM_READ_NBLK:
			event->operation = WAIT_MEM_READ_N;
			event->startCycle = sim_cycle;
			event->processingDone = 0;
			scheduleThroughNetwork(event, sim_cycle, data_packet_size, vc);
			dir_eventq_insert(event);
			break;	
		case MEM_WRITE_BACK:
			lat = cp_dir->blk_access_fn(Write, CACHE_BADDR(cp_dir,event->addr), cp_dir->bsize, NULL, sim_cycle, event->tempID);
			free_event(event);
			break;
		case L2_PREFETCH	:
			event->parent_operation = event->operation;
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{
				free_event(event);
				return 1;
			}
			else
			{
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				if(!matchnum && !md_text_addr(event->addr, tempID))
				{ /* No match in L2 MSHR */
					if(!cache_dl2->mshr->freeEntries)
					{ /* L2 mshr is full */
						L2_mshr_full_prefetch ++;
						free_event(event);
						return 1;
					}	
					l2_prefetch_num ++;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL); 
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					free_event(event);
					return 1;
				}
			}
			break;                             

		case MISS_IL1		:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* Hit */
				if (blk->way_prev && cp_dir->policy == LRU)
					update_way_list (&cp_dir->sets[set_dir], blk, Head);

				if(blk->dir_state[block_offset] == DIR_TRANSITION)
					panic("DIR: This event could not be in transition");

				if(blk->dir_sharer[block_offset][0] != 0 || blk->dir_sharer[block_offset][1] != 0 || blk->dir_sharer[block_offset][2] != 0 || blk->dir_sharer[block_offset][3] != 0 || /*blk->state != MESI_SHARED ||*/ (blk->status & CACHE_BLK_DIRTY))
					panic("DIR: IL1 block can not have sharers list or modified");

				if(collect_stats)
				{
					cp_dir->data_access ++;
					cp_dir->hits ++;
				}
				event->operation = ACK_DIR_IL1;
				event->src1 += event->des1;
				event->des1 = event->src1 - event->des1;
				event->src1 = event->src1 - event->des1;
				event->src2 += event->des2;
				event->des2 = event->src2 - event->des2;
				event->src2 = event->src2 - event->des2;
				event->processingDone = 0;
				event->startCycle = sim_cycle;
				event->parent = NULL;
				event->blk_dir = NULL;
				event->arrival_time = sim_cycle - event->arrival_time;
				scheduleThroughNetwork(event, sim_cycle, data_packet_size, vc);
				dir_eventq_insert(event);
				return 1;
			}
			else
			{   /* L2 Miss */
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				event->L2miss_flag = 1;
				event->L2miss_stated = sim_cycle;
				if(!matchnum)
				{ /* No match in L2 MSHR */
#ifdef DIR_FIFO_FULL
					if(!cache_dl2->mshr->freeEntries)
					{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
						if(!last_L2_mshr_full)
						{
							L2_mshr_full ++;
							last_L2_mshr_full = sim_cycle;
						}
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
						if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
						{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
							dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
							//dir_fifo_nack[event->des1*mesh_size+event->des2]
							event->src1 += event->des1;
							event->des1 = event->src1 - event->des1;
							event->src1 = event->src1 - event->des1;
							event->src2 += event->des2;
							event->des2 = event->src2 - event->des2;
							event->src2 = event->src2 - event->des2;
							event->operation = NACK;
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
							dir_eventq_insert(event);
							return 1;
						}
						else	
						{
							dir_fifo_full[event->des1*mesh_size+event->des2] ++;
							event->when = sim_cycle + RETRY_TIME;
							flip_counter ++;
							event->flip_flag = 1;
							dir_eventq_insert(event);
							return 1;
						}
					} 
					else
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
						if(last_L2_mshr_full)
						{
							Stall_L2_mshr += sim_cycle - last_L2_mshr_full;
							last_L2_mshr_full = 0;
						}
					}
#else
					if(!cache_dl2->mshr->freeEntries)
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}	
#endif
					if(collect_stats)
						cache_dl2->misses++;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL); 
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					if(collect_stats)
						cache_dl2->in_mshr ++;

					event_list_insert(event, matchnum);
					return 1;
				}
			}
#ifdef PREFETCH_OPTI
		case L1_PREFETCH	:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(event->reqNetTime == 0)
				event->reqNetTime = sim_cycle;
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
				{/* Hit */
				if (blk->way_prev && cp_dir->policy == LRU)
					update_way_list (&cp_dir->sets[set_dir], blk, Head);

				blk->dir_prefetch[block_offset][tempID] = 1; //dir_prefetch indicate if the processor prefetch this block before
				blk->dir_prefetch_num[block_offset] ++;
				/*Three places to reset: 1. Reset when the directory receives the request from the same processor to the same block
				  2. Reset when L1 writeback this block (silent drop does not have this case)
				  3. Reset when L1 sends the prefetch useful update request */
				event->src1 += event->des1;
				event->des1 = event->src1 - event->des1;
				event->src1 = event->src1 - event->des1;
				event->src2 += event->des2;
				event->des2 = event->src2 - event->des2;
				event->src2 = event->src2 - event->des2;
				event->operation = ACK_DIR_READ_EXCLUSIVE;  //return exclusive state in L1 cache
				event->processingDone = 0;
				event->startCycle = sim_cycle;
				event->parent = NULL;
				event->blk_dir = NULL;
				event->arrival_time = sim_cycle - event->arrival_time;
				scheduleThroughNetwork(event, sim_cycle, data_packet_size, vc);
				dir_eventq_insert(event);
				return 1;
			}
			else
			{   /* L2 Miss */
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				event->L2miss_flag = 1;
				event->L2miss_stated = sim_cycle;
				if(event->isSyncAccess)
					Sync_L2_miss ++;
				if(!matchnum)
				{ /* No match in L2 MSHR */
#ifdef DIR_FIFO_FULL
					if(!cache_dl2->mshr->freeEntries)
					{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
						if(!last_L2_mshr_full)
						{
							L2_mshr_full ++;
							last_L2_mshr_full = sim_cycle;
						}
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
						if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
						{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
							dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
							//dir_fifo_nack[event->des1*mesh_size+event->des2]
							event->src1 += event->des1;
							event->des1 = event->src1 - event->des1;
							event->src1 = event->src1 - event->des1;
							event->src2 += event->des2;
							event->des2 = event->src2 - event->des2;
							event->src2 = event->src2 - event->des2;
							event->pendingInvAck = 0;
							event->operation = NACK;
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
							dir_eventq_insert(event);
							return 1;
						}
						else	
						{
							dir_fifo_full[event->des1*mesh_size+event->des2] ++;
							event->when = sim_cycle + RETRY_TIME;
							event->flip_flag = 1;
							flip_counter ++;
							dir_eventq_insert(event);
							return 1;
						}
					} 
					else
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
						if(last_L2_mshr_full)
						{
							Stall_L2_mshr += sim_cycle - last_L2_mshr_full;
							last_L2_mshr_full = 0;
						}
					}
#else
					if(!cache_dl2->mshr->freeEntries)
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}	
#endif
					if(collect_stats)
					{
						cache_dl2->misses ++;
						cache_dl2->dmisses ++;
						involve_2_hops_miss ++;
					}
					event->reqAtDirTime = sim_cycle;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL); 
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					event->isReqL2SecMiss = 1;
					if(collect_stats)
					{
						cache_dl2->in_mshr ++;
						cache_dl2->din_mshr ++;
					}
					event_list_insert(event, matchnum);
					return 1;
				}
			}

			
		break;
		case READ_COHERENCE_UPDATE :
			/* received the read coherence update request for prefetch */	
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* Hit */
				if (blk->way_prev && cp_dir->policy == LRU)
					update_way_list (&cp_dir->sets[set_dir], blk, Head);
				/* See the status of directory */
#ifdef DIR_FIFO_FULL
				if(blk->dir_state[block_offset] == DIR_TRANSITION 
#ifdef READ_PERMIT
						|| (blk->ReadPending[block_offset] != -1)
#endif
				  )
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
					if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
					{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
						dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
						//dir_fifo_nack[event->des1*mesh_size+event->des2]
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						event->operation = NACK;
						event->processingDone = 0;
						event->startCycle = sim_cycle;
						scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
					else	
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] ++;
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
				} 
				else
					dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
#else		    
				if(blk->dir_state[block_offset] == DIR_TRANSITION) 
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
					event->when = sim_cycle + RETRY_TIME;
					flip_counter ++;
					event->flip_flag = 1;
					dir_eventq_insert(event);
					if(collect_stats)
						cp_dir->dir_access ++;
					return 1;
				} 
#endif			    
				if((!IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid)) && (blk->dir_dirty[block_offset] != -1))
				{
#ifdef L1_STREAM_PREFETCHER
					if(event->prefetch_next == 4)
					{
						write_back_early ++;
						dir_fifo_full[event->des1*mesh_size+event->des2] ++;
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}
					else
#endif
						panic("DIR: Block can not be dirty state when it is not shared!");
				}

				if(blk->dir_prefetch[block_offset][tempID]) //prefetch bit reset case 3
				{
					blk->dir_prefetch[block_offset][tempID] = 0;	
					blk->dir_prefetch_num[block_offset] --;
					if(blk->dir_prefetch_num[block_offset] < 0)
						panic("prefetch number doesn't match!\n");
				}

				if(collect_stats)
				{
					cp_dir->hits ++;
					cp_dir->dhits ++;
				}
				int exclusive_clean = 0;
				/* Find if director is in modified/exclusive state */
				if(IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid) && blk->dir_blk_state[block_offset] != MESI_SHARED)
				{/* dirty or exclusive */
					ReadDowngrade_EM ++;	
					IsNeighbor(blk->dir_sharer[block_offset], tempID, MISS_READ, event->addr);
					blk->dir_state[block_offset] = DIR_TRANSITION;
					blk->ptr_cur_event[block_offset] = event;
					event->blk_dir = blk;
					struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
					if(new_event == NULL)       panic("Out of Virtual Memory");
					new_event->input_buffer_full = 0;
					new_event->req_time = 0;
					new_event->conf_bit = -1;
					new_event->pending_flag = 0;
					new_event->op = event->op;
					new_event->isPrefetch = 0;
					new_event->operation = INV_MSG_READ;
					new_event->src1 = event->des1;
					new_event->src2 = event->des2;
					new_event->des1 = Threadid/mesh_size+MEM_LOC_SHIFT;
					new_event->des2 = Threadid%mesh_size;
					new_event->processingDone = 0;
					new_event->startCycle = sim_cycle;
					new_event->parent = event;
					new_event->tempID = event->tempID;
					new_event->resend = 0;
					new_event->blk_dir = blk;
					new_event->cp = event->cp;
					new_event->store_cond = event->store_cond;
					new_event->addr = event->addr;
					new_event->vp = NULL;
					new_event->nbytes = event->cp->bsize;
					new_event->udata = NULL;
					new_event->cmd = Write;
					new_event->rs = event->rs;
					new_event->started = sim_cycle;
					new_event->spec_mode = 0;
					new_event->L2miss_flag = event->L2miss_flag;
#ifdef COHERENCE_MODIFIED
#ifdef INV_ACK_CON
					if(blk->dir_blk_state[block_offset] == MESI_EXCLUSIVE)
					{
						if((new_event->src1*mesh_size+new_event->src2 != new_event->des1*mesh_size+new_event->des2) && ((!mystricmp(network_type, "FSOI")) || (!mystricmp(network_type, "HYBRID") && ((abs(new_event->src1 - new_event->des1) + abs(new_event->src2 - new_event->des2)) > 1))))
						{
#if 0
							new_event->data_reply = 0;
							exclusive_clean = 1;
							total_exclusive_conf ++;
							struct DIRECTORY_EVENT *new_event2 = allocate_event(event->isSyncAccess);
							if(new_event2 == NULL)       panic("Out of Virtual Memory");
							if(collect_stats)
								cp_dir->dir_access ++;
							new_event2->input_buffer_full = 0;
							new_event2->req_time = 0;
							new_event2->conf_bit = -1;
							new_event2->pending_flag = 0;
							new_event2->op = event->op;
							new_event2->isPrefetch = 0;
							new_event2->operation = ACK_MSG_READUPDATE;
							new_event2->store_cond = event->store_cond;
							new_event2->src1 = new_event->des1;
							new_event2->src2 = new_event->des2;
							new_event2->des1 = new_event->src1;
							new_event2->des2 = new_event->src2;
							new_event2->processingDone = 0;
							new_event2->startCycle = sim_cycle;
							new_event2->parent = event;
							new_event2->tempID = event->tempID;
							new_event2->resend = 0;
							new_event2->blk_dir = blk;
							new_event2->cp = event->cp;
							new_event2->addr = event->addr;
							new_event2->vp = NULL;
							new_event2->nbytes = event->cp->bsize;
							new_event2->udata = NULL;
							new_event2->cmd = Read;
							new_event2->rs = event->rs;
							new_event2->started = sim_cycle;
							new_event2->spec_mode = 0;
							new_event2->L2miss_flag = event->L2miss_flag;
							new_event2->data_reply = 0;
							popnetMsgNo ++;
							new_event2->popnetMsgNo = popnetMsgNo;
							new_event2->when = WAIT_TIME;
							dir_eventq_insert(new_event2);
#else
							new_event->data_reply = 0;
							exclusive_clean = 1;
							total_exclusive_conf ++;
#endif
						}
						else	
							new_event->data_reply = 1;
					}
					else
#endif
#endif
						new_event->data_reply = 1;
					event->childCount++;
					event->dirty_flag = 1;
					if(event->rs)
						if(event->rs->op == LL)
							load_link_exclusive ++;

					/*go to network*/
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);

					dir_eventq_insert(new_event);
#ifdef COHERENCE_MODIFIED
					if(!exclusive_clean) //indicate this msg is downgrade msg for a clean copy
#endif
					return 1;
				}

				/* Either there is no sharers (E mode) or there are sharers (S mode)*/
				if(collect_stats)
				{
				}
				blk->dir_state[block_offset] = DIR_STABLE;
				blk->ptr_cur_event[block_offset] = NULL;
				if(blk->prefetch_modified[block_offset] || blk->prefetch_invalidate[block_offset])
				{
					if(IsShared(blk->dir_sharer[block_offset], tempID))
						event->operation = READ_DATA_REPLY_SHARED;
					else
						event->operation = READ_DATA_REPLY_EXCLUSIVE;
					packet_size = data_packet_size;
					ReadDataReply_CS ++;
				}
				else
				{
					if(IsShared(blk->dir_sharer[block_offset], tempID))
						event->operation = READ_META_REPLY_SHARED;
					else
						event->operation = READ_META_REPLY_EXCLUSIVE;
					packet_size = meta_packet_size;
					ReadMetaReply_CS ++;
				}
				event->reqAckTime = sim_cycle;
				if(!md_text_addr(addr, tempID))
				{
					blk->dir_sharer[block_offset][tempID/64] = (IsShared(blk->dir_sharer[block_offset], tempID) ? blk->dir_sharer[block_offset][tempID/64] : 0) | (unsigned long long int)1<<(tempID%64);
					blk->dir_blk_state[block_offset] = IsShared(blk->dir_sharer[block_offset], tempID) ? MESI_SHARED : MESI_EXCLUSIVE;
				}
				if(blk->dir_prefetch_num[block_offset] == 0)
				{
					blk->prefetch_invalidate[block_offset] = 0;
					blk->prefetch_modified[block_offset] = 0;
				}
				event->src1 += event->des1;
				event->des1 = event->src1 - event->des1;
				event->src1 = event->src1 - event->des1;
				event->src2 += event->des2;
				event->des2 = event->src2 - event->des2;
				event->src2 = event->src2 - event->des2;
				event->processingDone = 0;
				event->startCycle = sim_cycle;
				event->parent = NULL;
				event->blk_dir = NULL;
				event->arrival_time = sim_cycle - event->arrival_time;
				scheduleThroughNetwork(event, sim_cycle, packet_size, vc);
				dir_eventq_insert(event);
#ifdef LOCK_CACHE_REGISTER
				/* update the lock register if necessary */
				if(event->store_cond == MY_LDL_L) // && (bool_value)) //FIXME: check the value if bool or not
					LockRegesterCheck(event);
#endif				
				return 1;
			}
			else
			{   /* L2 Miss */
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				event->L2miss_flag = 1;
				event->L2miss_stated = sim_cycle;
				if(event->isSyncAccess)
					Sync_L2_miss ++;
				if(!matchnum)
				{ /* No match in L2 MSHR */
#ifdef DIR_FIFO_FULL
					if(!cache_dl2->mshr->freeEntries)
					{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
						if(!last_L2_mshr_full)
						{
							L2_mshr_full ++;
							last_L2_mshr_full = sim_cycle;
						}
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
						if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
						{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
							dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
							//dir_fifo_nack[event->des1*mesh_size+event->des2]
							event->src1 += event->des1;
							event->des1 = event->src1 - event->des1;
							event->src1 = event->src1 - event->des1;
							event->src2 += event->des2;
							event->des2 = event->src2 - event->des2;
							event->src2 = event->src2 - event->des2;
							event->pendingInvAck = 0;
							event->operation = NACK;
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
							dir_eventq_insert(event);
							return 1;
						}
						else	
						{
							dir_fifo_full[event->des1*mesh_size+event->des2] ++;
							event->when = sim_cycle + RETRY_TIME;
							event->flip_flag = 1;
							flip_counter ++;
							dir_eventq_insert(event);
							return 1;
						}
					} 
					else
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
						if(last_L2_mshr_full)
						{
							Stall_L2_mshr += sim_cycle - last_L2_mshr_full;
							last_L2_mshr_full = 0;
						}
					}
#else
					if(!cache_dl2->mshr->freeEntries)
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}	
#endif
					if(collect_stats)
					{
						cache_dl2->misses ++;
						cache_dl2->dmisses ++;
						involve_2_hops_miss ++;
					}
					ReadL2Miss ++;
					event->reqAtDirTime = sim_cycle;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL); 
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					event->isReqL2SecMiss = 1;
					if(collect_stats)
					{
						cache_dl2->in_mshr ++;
						cache_dl2->din_mshr ++;
					}
					event_list_insert(event, matchnum);
					return 1;
				}
			}
			
		break;
		case WRITE_COHERENCE_UPDATE	:
			/* received the write coherence update request for prefetch */	
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* Hit */
				unsigned long long int *multiple_sharers = (unsigned long long int *)calloc(4, sizeof(unsigned long long int));
				int currentThreadId = (event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2;
				if(event->L2miss_flag && event->L2miss_stated)
					event->L2miss_complete = sim_cycle;
				if (blk->way_prev && cp_dir->policy == LRU)
					update_way_list (&cp_dir->sets[set_dir], blk, Head);
				event->isReqL2Hit = 1;
#ifdef REAL_LOCK
				if(blk->dir_state[block_offset] == DIR_TRANSITION || (blk->dir_state[block_offset] == DIR_STABLE && blk->dir_dirty[block_offset] != -1 && blk->dir_dirty[block_offset] != tempID)
#ifdef SERIALIZATION_ACK
						|| blk->pendingInvAck[block_offset] 
#endif
				  ) 
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					if(event->store_cond == MY_STL_C)
					{
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						event->operation = FAIL;
						event->processingDone = 0;
						event->startCycle = sim_cycle;
						store_conditional_failed ++;
						event->when = sim_cycle;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
				}
#endif
#ifdef DIR_FIFO_FULL
				if(blk->dir_state[block_offset] == DIR_TRANSITION
#ifdef SERIALIZATION_ACK
						|| blk->pendingInvAck[block_offset] 
#endif
				  ) 
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
					if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
					{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
						dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						event->operation = NACK;
						event->processingDone = 0;
						event->startCycle = sim_cycle;
						scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
					else	
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] ++;
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
				} 
				else
					dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
#else		    

				/* See the status of directory */
				if(blk->dir_state[block_offset] == DIR_TRANSITION
#ifdef SERIALIZATION_ACK
						|| blk->pendingInvAck[block_offset] 
#endif
				  )
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
					event->when = sim_cycle + RETRY_TIME;
					flip_counter ++;
					event->flip_flag = 1;
					dir_eventq_insert(event);
					if(collect_stats)
						cp_dir->dir_access ++;
					return 1;
				} 
#endif
				event->reqAtDirTime = sim_cycle;

#ifdef SERIALIZATION_ACK
				if(!Is_ExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid) && Is_Shared(blk->dir_sharer[block_offset], tempID))
				{
					if(isInvCollectTableFull((event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2) || blk->pendingInvAck[block_offset])
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}	
					multiple_sharers = blk->dir_sharer[block_offset];
				}
#endif
				if((!IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid)) && (blk->dir_dirty[block_offset] != -1))
					panic("DIR: Block can not be dirty state when it is not exclusive!");

				int isExclusiveOrDirty = Is_ExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid);

				if(blk->dir_prefetch[block_offset][tempID]) //prefetch bit reset case 3
				{
					blk->dir_prefetch[block_offset][tempID] = 0;	
					blk->dir_prefetch_num[block_offset] --;
					if(blk->dir_prefetch_num[block_offset] < 0)
						panic("prefetch number doesn't match!\n");
				}

#ifdef SERIALIZATION_ACK
				//if(!IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid))
				if(!isExclusiveOrDirty || (isExclusiveOrDirty && Threadid == tempID))
#else
					if(!IsShared(blk->dir_sharer[block_offset], tempID))
#endif
					{
						if(collect_stats)
						{
							if(blk->ever_dirty == 1)
								involve_2_hop_wb ++;
							if(blk->ever_touch == 1)
								involve_2_hop_touch ++;
							if(event->parent_operation == MISS_WRITE)
							{
								cp_dir->data_access ++;
								involve_2_hops_wm ++;
								WM_Clean ++;
							}
							else
							{
								involve_2_hops_upgrade ++;
								cp_dir->dir_access ++;
							}
						}
						blk->dir_state[block_offset] = DIR_STABLE;
						blk->dir_blk_state[block_offset] = MESI_MODIFIED;
						blk->ptr_cur_event[block_offset] = NULL;
						event->reqAckTime = sim_cycle;
						if(blk->prefetch_invalidate[block_offset] || blk->prefetch_modified[block_offset])
						{
							event->operation = WRITE_DATA_REPLY;
							packet_size = data_packet_size;
							WriteDataReply_C ++;
						}
						else
						{
							event->operation = WRITE_META_REPLY;
							packet_size = meta_packet_size;
							WriteMetaReply_C ++;
						}

						blk->dir_dirty[block_offset] = tempID;
						blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						if(blk->dir_prefetch_num[block_offset] == 0)
						{
							blk->prefetch_invalidate[block_offset] = 0;
							blk->prefetch_modified[block_offset] = 0;
						}
						
						if(!mystricmp (network_type, "FSOI") || (!mystricmp (network_type, "HYBRID")))
						{
#ifdef EUP_NETWORK
						if(event->early_flag == 1)
							EUP_entry_dellocate((event->src1-MEM_LOC_SHIFT)*mesh_size+event->src2, event->addr>>cache_dl2->set_shift);
						else
						{
#endif
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							event->parent = NULL;
							event->blk_dir = NULL;
							event->pendingInvAck = (multiple_sharers[0] != 0 || multiple_sharers[1] != 0 || multiple_sharers[2] != 0 || multiple_sharers[3] != 0);
							event->arrival_time = sim_cycle - event->arrival_time;
							scheduleThroughNetwork(event, sim_cycle, packet_size, vc);
							dir_eventq_insert(event);
#ifdef EUP_NETWORK
						}
#endif
						}
						else
						{
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							event->parent = NULL;
							event->blk_dir = NULL;
							event->pendingInvAck = (multiple_sharers[0] != 0 || multiple_sharers[1] != 0 || multiple_sharers[2] != 0 || multiple_sharers[3] != 0);
							event->arrival_time = sim_cycle - event->arrival_time;
							scheduleThroughNetwork(event, sim_cycle, packet_size, vc);
							dir_eventq_insert(event);
						}

#ifdef SERIALIZATION_ACK
						if(multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0)
#endif
							return 1;
					}

				/* Invalidate multiple sharers */
				event->blk_dir = blk;
				if(multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0)
				{
					blk->dir_state[block_offset] = DIR_TRANSITION;
					blk->ptr_cur_event[block_offset] = event;
					event->blk_dir = blk;
				}
				event->childCount = 0;

				if(multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0)
					multiple_sharers = (blk->dir_sharer[block_offset]);
				IsNeighbor(multiple_sharers, tempID, event->operation, event->addr);

				int multisharers_flag = 0;
				if(IsShared(multiple_sharers, tempID) > 1) 
					multisharers_flag = 1;

				for (Threadid = 0; Threadid < numcontexts; Threadid++)
				{
					if((((multiple_sharers[Threadid/64]) & ((unsigned long long int)1 << (Threadid%64))) == ((unsigned long long int)1 << (Threadid%64))) && (Threadid != tempID))
					{
						struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);

						if(new_event == NULL) 	panic("Out of Virtual Memory");
						WriteInvalidate_SEM ++;
						new_event->req_time = 0;
						new_event->conf_bit = -1;
						new_event->input_buffer_full = 0;
						new_event->pending_flag = 0;
						new_event->op = event->op;
						new_event->isPrefetch = 0;
						new_event->L2miss_flag = event->L2miss_flag;
#ifdef LOCK_CACHE_REGISTER
						if(event->store_cond == MY_STL_C && multisharers_flag) //FIXME: need to check if the value is boolean or not
						{
							new_event->operation = INV_MSG_UPDATE;
							new_event->conf_bit = event->childCount;
						}
						else
#endif
							new_event->operation = INV_MSG_WRITE;
						new_event->isExclusiveOrDirty = isExclusiveOrDirty;
						new_event->src1 = currentThreadId/mesh_size+MEM_LOC_SHIFT;
						new_event->src2 = currentThreadId%mesh_size;
						new_event->des1 = Threadid/mesh_size+MEM_LOC_SHIFT;
						new_event->des2 = Threadid%mesh_size;
						new_event->processingDone = 0;
						new_event->startCycle = sim_cycle;
						new_event->parent = event;
						new_event->tempID = tempID;
						new_event->resend = 0;
						new_event->blk_dir = blk;
						new_event->cp = event->cp;
						new_event->addr = event->addr;
						new_event->vp = NULL;
						new_event->nbytes = event->cp->bsize;
						new_event->udata = NULL;
						new_event->cmd = Write;
						new_event->rs = event->rs;
						new_event->started = sim_cycle;
						new_event->store_cond = event->store_cond;
						new_event->spec_mode = 0;
							event->childCount++;
						if(!mystricmp(network_type, "FSOI") || (!mystricmp(network_type, "HYBRID")))
						{
#ifdef INV_ACK_CON
						int thread_id;
						if(Is_Shared(multiple_sharers, tempID) > 1) 
						{
							if((new_event->src1*mesh_size+new_event->src2 != new_event->des1*mesh_size+new_event->des2) && ((!mystricmp(network_type, "FSOI")) || (!mystricmp(network_type, "HYBRID") && ((abs(new_event->src1 - new_event->des1) + abs(new_event->src2 - new_event->des2)) > 1))))
							{
								new_event->data_reply = 0;
								struct DIRECTORY_EVENT *new_event2 = allocate_event(event->isSyncAccess);
								if(new_event2 == NULL)       panic("Out of Virtual Memory");
								if(collect_stats)
									cp_dir->dir_access ++;
								new_event2->input_buffer_full = 0;
								new_event2->req_time = 0;
								new_event2->conf_bit = -1;
								new_event2->pending_flag = 0;
								new_event2->op = event->op;
								new_event2->isPrefetch = 0;
#ifdef LOCK_CACHE_REGISTER
								if(event->store_cond == MY_STL_C && multisharers_flag) //FIXME: need to check if the value if boolean or not
										new_event2->operation = ACK_MSG_UPDATE;
								else
#endif
								new_event2->operation = ACK_MSG_WRITEUPDATE;
								new_event2->store_cond = event->store_cond;
								new_event2->src1 = new_event->des1;
								new_event2->src2 = new_event->des2;
								new_event2->des1 = new_event->src1;
								new_event2->des2 = new_event->src2;
								new_event2->processingDone = 0;
								new_event2->startCycle = sim_cycle;
								new_event2->parent = event;
								new_event2->tempID = event->tempID;
								new_event2->resend = 0;
								new_event2->blk_dir = blk;
								new_event2->cp = event->cp;
								new_event2->addr = event->addr;
								new_event2->vp = NULL;
								new_event2->nbytes = event->cp->bsize;
								new_event2->udata = NULL;
								new_event2->cmd = Write;
								new_event2->rs = event->rs;
								new_event2->started = sim_cycle;
								new_event2->spec_mode = 0;
								new_event2->L2miss_flag = event->L2miss_flag;
								new_event2->data_reply = 0;
								popnetMsgNo ++;
								new_event2->popnetMsgNo = popnetMsgNo;
								new_event2->when = WAIT_TIME;
								dir_eventq_insert(new_event2);
							}
							else	
								new_event->data_reply = 1;
						}
						else	
							new_event->data_reply = 1;
#else
						new_event->data_reply = 1;
#endif
						}
						else 
							new_event->data_reply = 1;
						event->dirty_flag = 1;
						/*go to network*/
						scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);

						dir_eventq_insert(new_event);
					}
				}
				sharer_distr[event->childCount] ++;
				IsNeighbor_sharer(multiple_sharers, tempID);
#ifdef SERIALIZATION_ACK
				//if(!isExclusiveOrDirty && event->childCount)
				if(event->childCount >= 1 && !isExclusiveOrDirty)
				{
					int final_ack_conf = 0;
					if(event->operation == ACK_DIR_WRITEUPDATE)
						final_ack_conf = 1;
					else	
						final_ack_conf = 0;

					if(event->operation == ACK_DIR_WRITE)
						write_shared_used_conf ++;
					newInvCollect((event->src1-MEM_LOC_SHIFT)*mesh_size+event->src2, event->addr, event->childCount, tempID, final_ack_conf);
					blk->pendingInvAck[block_offset] = 1;
					if(!mystricmp (network_type, "MESH"))
					{
						if(event->operation == ACK_DIR_WRITEUPDATE)
						{
							totalUpgradesUsable++;
							if(thecontexts[tempID]->m_shSQNum > m_shSQSize-3)
								totalUpgradesBeneficial++;
						}
					}
				}
#endif
				return 1;
			}
			else
			{   /* L2 Miss */
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				event->L2miss_flag = 1;
				event->L2miss_stated = sim_cycle;
				if(event->isSyncAccess)
					Sync_L2_miss ++;
				if(!matchnum)
				{ /* No match in L2 MSHR */
#ifdef DIR_FIFO_FULL
					if(!cache_dl2->mshr->freeEntries)
					{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
						if(!last_L2_mshr_full)
						{
							L2_mshr_full ++;
							last_L2_mshr_full = sim_cycle;
						}
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
						if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
						{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
							dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
							event->src1 += event->des1;
							event->des1 = event->src1 - event->des1;
							event->src1 = event->src1 - event->des1;
							event->src2 += event->des2;
							event->des2 = event->src2 - event->des2;
							event->src2 = event->src2 - event->des2;
							event->operation = NACK;
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
							dir_eventq_insert(event);
							return 1;
						}
						else	
						{
							dir_fifo_full[event->des1*mesh_size+event->des2] ++;
							event->when = sim_cycle + RETRY_TIME;
							flip_counter ++;
							event->flip_flag = 1;
							dir_eventq_insert(event);
							return 1;
						}
					} 
					else
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
						if(last_L2_mshr_full)
						{
							Stall_L2_mshr += sim_cycle - last_L2_mshr_full;
							last_L2_mshr_full = 0;
						}
					}
#else
					if(!cache_dl2->mshr->freeEntries)
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}	
#endif
					data_private_write ++;
					if(collect_stats)
					{
						cache_dl2->misses ++;
						cache_dl2->dmisses ++;
						involve_2_hops_miss ++;
					}
					WriteL2Miss ++;
					event->reqAtDirTime = sim_cycle;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					if(event->operation != WRITE_UPDATE)
						WM_Miss ++;
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL);
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					event->isReqL2SecMiss = 1;
					if(collect_stats)
					{
						cache_dl2->in_mshr ++;
						cache_dl2->din_mshr ++;
					}
					event_list_insert(event, matchnum);
					return 1;	
				}
			}
		break;
#endif
		case MISS_READ		:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(event->reqNetTime == 0)
				event->reqNetTime = sim_cycle;
			if(findCacheStatus(cp_dir/*dl2*/, set_dir/*dl2*/, tag_dir/*dl2*/, hindex_dir/*dl2*/, &blk))
			{/* Hit */

				if (blk->way_prev && cp_dir->policy == LRU)
					update_way_list (&cp_dir->sets[set_dir], blk, Head);
				/* See the status of directory */
				event->isReqL2Hit = 1;
				if(event->L2miss_flag && event->L2miss_stated)
					event->L2miss_complete = sim_cycle;
#ifdef DIR_FIFO_FULL
				if(blk->dir_state[block_offset] == DIR_TRANSITION 
#ifdef READ_PERMIT
						|| (blk->ReadPending[block_offset] != -1)
#endif
				  )
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
					if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
					{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
						dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
						//dir_fifo_nack[event->des1*mesh_size+event->des2]
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						event->operation = NACK;
						event->processingDone = 0;
						event->startCycle = sim_cycle;
						scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
					else	
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] ++;
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
				} 
				else
					dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
#else		    
				if(blk->dir_state[block_offset] == DIR_TRANSITION) 
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
					event->when = sim_cycle + RETRY_TIME;
					flip_counter ++;
					event->flip_flag = 1;
					dir_eventq_insert(event);
					if(collect_stats)
						cp_dir->dir_access ++;
					return 1;
				} 
#endif			    
				if((!IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid)) && (blk->dir_dirty[block_offset] != -1))
				{
#ifdef L1_STREAM_PREFETCHER
					if(event->prefetch_next == 4)
					{
						write_back_early ++;
						dir_fifo_full[event->des1*mesh_size+event->des2] ++;
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}
					else
#endif
						panic("DIR: Block can not be dirty state when it is not shared!");
				}

#ifdef PREFETCH_OPTI
				if(blk->dir_prefetch[block_offset][tempID]) //prefetch bit reset case 1
					blk->dir_prefetch[block_offset][tempID] = 0;	
#endif
				if(collect_stats)
				{
					cp_dir->hits ++;
					cp_dir->dhits ++;

					totalL2ReadReqHits++;
					if(blk->dir_sharer[block_offset][0] == 0 && blk->dir_sharer[block_offset][1] == 0 && blk->dir_sharer[block_offset][2] == 0 && blk->dir_sharer[block_offset][3] == 0)
						totalL2ReadReqNoSharer++;
					else if(IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid))
					{
						event->isReqL2Inv = 1;
						totalL2ReadReqDirty++;
					}
					else
						totalL2ReadReqShared++;
				}
				event->reqAtDirTime = sim_cycle;
				int exclusive_clean = 0;
				/* Find if director is in modified/exclusive state */
				if(IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid) && blk->dir_blk_state[block_offset] != MESI_SHARED)
				{/* dirty or exclusive */
					if(collect_stats)
						involve_4_hops ++;
					IsNeighbor(blk->dir_sharer[block_offset], tempID, MISS_READ, event->addr);
					blk->dir_state[block_offset] = DIR_TRANSITION;
					blk->ptr_cur_event[block_offset] = event;
					event->blk_dir = blk;
					struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
					if(new_event == NULL)       panic("Out of Virtual Memory");
					if(collect_stats)
						cp_dir->dir_access ++;
					new_event->input_buffer_full = 0;
					new_event->req_time = 0;
					new_event->conf_bit = -1;
					new_event->pending_flag = 0;
					new_event->op = event->op;
					new_event->isPrefetch = 0;
					new_event->operation = INV_MSG_READ;
					new_event->src1 = event->des1;
					new_event->src2 = event->des2;
					new_event->des1 = Threadid/mesh_size+MEM_LOC_SHIFT;
					new_event->des2 = Threadid%mesh_size;
					new_event->processingDone = 0;
					new_event->startCycle = sim_cycle;
					new_event->parent = event;
					new_event->tempID = event->tempID;
					new_event->resend = 0;
					new_event->blk_dir = blk;
					new_event->cp = event->cp;
					new_event->store_cond = event->store_cond;
					new_event->addr = event->addr;
					new_event->vp = NULL;
					new_event->nbytes = event->cp->bsize;
					new_event->udata = NULL;
					new_event->cmd = Write;
					new_event->rs = event->rs;
					new_event->started = sim_cycle;
					new_event->spec_mode = 0;
					new_event->L2miss_flag = event->L2miss_flag;
#ifdef COHERENCE_MODIFIED
#ifdef INV_ACK_CON
					if(blk->dir_blk_state[block_offset] == MESI_EXCLUSIVE)
					{
						if((new_event->src1*mesh_size+new_event->src2 != new_event->des1*mesh_size+new_event->des2) && ((!mystricmp(network_type, "FSOI")) || (!mystricmp(network_type, "HYBRID") && ((abs(new_event->src1 - new_event->des1) + abs(new_event->src2 - new_event->des2)) > 1))))
						{
#if 0
							new_event->data_reply = 0;
							exclusive_clean = 1;
							total_exclusive_conf ++;
							struct DIRECTORY_EVENT *new_event2 = allocate_event(event->isSyncAccess);
							if(new_event2 == NULL)       panic("Out of Virtual Memory");
							if(collect_stats)
								cp_dir->dir_access ++;
							new_event2->input_buffer_full = 0;
							new_event2->req_time = 0;
							new_event2->conf_bit = -1;
							new_event2->pending_flag = 0;
							new_event2->op = event->op;
							new_event2->isPrefetch = 0;
							new_event2->operation = ACK_MSG_READUPDATE;
							new_event2->store_cond = event->store_cond;
							new_event2->src1 = new_event->des1;
							new_event2->src2 = new_event->des2;
							new_event2->des1 = new_event->src1;
							new_event2->des2 = new_event->src2;
							new_event2->processingDone = 0;
							new_event2->startCycle = sim_cycle;
							new_event2->parent = event;
							new_event2->tempID = event->tempID;
							new_event2->resend = 0;
							new_event2->blk_dir = blk;
							new_event2->cp = event->cp;
							new_event2->addr = event->addr;
							new_event2->vp = NULL;
							new_event2->nbytes = event->cp->bsize;
							new_event2->udata = NULL;
							new_event2->cmd = Read;
							new_event2->rs = event->rs;
							new_event2->started = sim_cycle;
							new_event2->spec_mode = 0;
							new_event2->L2miss_flag = event->L2miss_flag;
							new_event2->data_reply = 0;
							popnetMsgNo ++;
							new_event2->popnetMsgNo = popnetMsgNo;
							new_event2->when = WAIT_TIME;
							dir_eventq_insert(new_event2);
#else
							new_event->data_reply = 0;
							exclusive_clean = 1;
							total_exclusive_conf ++;
#endif
						}
						else	
							new_event->data_reply = 1;
					}
					else
#endif
#endif
						new_event->data_reply = 1;
					event->childCount++;
					event->dirty_flag = 1;
					if(event->rs)
						if(event->rs->op == LL)
							load_link_exclusive ++;

					/*go to network*/
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);

					dir_eventq_insert(new_event);
#ifdef COHERENCE_MODIFIED
					if(!exclusive_clean) //indicate this msg is downgrade msg for a clean copy
#endif
					return 1;
				}

				/* Either there is no sharers (E mode) or there are sharers (S mode)*/
				if(collect_stats)
				{
					cp_dir->data_access ++;
					involve_2_hops ++;
					if(blk->ever_dirty == 1)
						involve_2_hop_wb ++;
				}
				blk->dir_state[block_offset] = DIR_STABLE;
				blk->ptr_cur_event[block_offset] = NULL;
				if(event->rs)
					if(event->rs->op == LL)
						load_link_shared ++;
				event->operation = IsShared(blk->dir_sharer[block_offset], tempID) ? ACK_DIR_READ_SHARED : ACK_DIR_READ_EXCLUSIVE;
				event->reqAckTime = sim_cycle;
				if(!md_text_addr(addr, tempID))
				{
					blk->dir_sharer[block_offset][tempID/64] = (IsShared(blk->dir_sharer[block_offset], tempID) ? blk->dir_sharer[block_offset][tempID/64] : 0) | (unsigned long long int)1<<(tempID%64);
					blk->dir_blk_state[block_offset] = IsShared(blk->dir_sharer[block_offset], tempID) ? MESI_SHARED : MESI_EXCLUSIVE;
				}
				/*My stats*/
				if(event->operation == ACK_DIR_READ_EXCLUSIVE)
					blk->exclusive_time[block_offset] = sim_cycle;

				if(event->operation == ACK_DIR_READ_EXCLUSIVE && collect_stats && blk->ever_touch == 1)
					involve_2_hop_touch ++;
				event->src1 += event->des1;
				event->des1 = event->src1 - event->des1;
				event->src1 = event->src1 - event->des1;
				event->src2 += event->des2;
				event->des2 = event->src2 - event->des2;
				event->src2 = event->src2 - event->des2;
				event->processingDone = 0;
				event->startCycle = sim_cycle;
				event->parent = NULL;
				event->blk_dir = NULL;
				event->arrival_time = sim_cycle - event->arrival_time;
				scheduleThroughNetwork(event, sim_cycle, data_packet_size, vc);
				dir_eventq_insert(event);
#ifdef LOCK_CACHE_REGISTER
				/* update the lock register if necessary */
				if(event->store_cond == MY_LDL_L) // && (bool_value)) //FIXME: check the value if bool or not
					LockRegesterCheck(event);
#endif				
				return 1;
			}
			else
			{   /* L2 Miss */
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				event->L2miss_flag = 1;
				event->L2miss_stated = sim_cycle;
				if(event->isSyncAccess)
					Sync_L2_miss ++;
				if(!matchnum)
				{ /* No match in L2 MSHR */
#ifdef DIR_FIFO_FULL
					if(!cache_dl2->mshr->freeEntries)
					{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
						if(!last_L2_mshr_full)
						{
							L2_mshr_full ++;
							last_L2_mshr_full = sim_cycle;
						}
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
						if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
						{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
							dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
							//dir_fifo_nack[event->des1*mesh_size+event->des2]
							event->src1 += event->des1;
							event->des1 = event->src1 - event->des1;
							event->src1 = event->src1 - event->des1;
							event->src2 += event->des2;
							event->des2 = event->src2 - event->des2;
							event->src2 = event->src2 - event->des2;
							event->pendingInvAck = 0;
							event->operation = NACK;
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
							dir_eventq_insert(event);
							return 1;
						}
						else	
						{
							dir_fifo_full[event->des1*mesh_size+event->des2] ++;
							event->when = sim_cycle + RETRY_TIME;
							event->flip_flag = 1;
							flip_counter ++;
							dir_eventq_insert(event);
							return 1;
						}
					} 
					else
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
						if(last_L2_mshr_full)
						{
							Stall_L2_mshr += sim_cycle - last_L2_mshr_full;
							last_L2_mshr_full = 0;
						}
					}
#else
					if(!cache_dl2->mshr->freeEntries)
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}	
#endif
					if(collect_stats)
					{
						cache_dl2->misses ++;
						cache_dl2->dmisses ++;
						involve_2_hops_miss ++;
					}
					event->reqAtDirTime = sim_cycle;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL); 
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					event->isReqL2SecMiss = 1;
					if(collect_stats)
					{
						cache_dl2->in_mshr ++;
						cache_dl2->din_mshr ++;
					}
					event_list_insert(event, matchnum);
					return 1;
				}
			}

#ifdef COHERENCE_MODIFIED	
		case WRITE_INDICATE	:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(event->reqNetTime == 0)
				event->reqNetTime = sim_cycle;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* Hit */
				if((blk->dir_blk_state[block_offset] == MESI_EXCLUSIVE) && (((blk->dir_sharer[block_offset][tempID/64]) & ((unsigned long long int)1 << (tempID%64))) == ((unsigned long long int)1 << (tempID%64))))
				{
					blk->dir_blk_state[block_offset] = MESI_MODIFIED;
					total_exclusive_modified ++;
				}
				free_event(event);
			}
			else
			{   /* L2 Miss */
				panic("Write exclusive can not be a miss in L2 cache");
			}
		break;
#endif
		case WRITE_UPDATE	:	
		case MISS_WRITE		:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(event->reqNetTime == 0)
				event->reqNetTime = sim_cycle;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* Hit */
				unsigned long long int *multiple_sharers = (unsigned long long int *)calloc(4, sizeof(unsigned long long int));
				int currentThreadId = (event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2;
				if(event->L2miss_flag && event->L2miss_stated)
					event->L2miss_complete = sim_cycle;
				if (blk->way_prev && cp_dir->policy == LRU)
					update_way_list (&cp_dir->sets[set_dir], blk, Head);
				event->isReqL2Hit = 1;
#ifdef REAL_LOCK
				if(blk->dir_state[block_offset] == DIR_TRANSITION || (blk->dir_state[block_offset] == DIR_STABLE && blk->dir_dirty[block_offset] != -1 && blk->dir_dirty[block_offset] != tempID)
#ifdef SERIALIZATION_ACK
						|| blk->pendingInvAck[block_offset] 
#endif
				  ) 
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					if(event->store_cond == MY_STL_C)
					{
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						event->operation = FAIL;
						event->processingDone = 0;
						event->startCycle = sim_cycle;
						store_conditional_failed ++;
						event->when = sim_cycle;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
				}
#endif
#ifdef DIR_FIFO_FULL
				if(blk->dir_state[block_offset] == DIR_TRANSITION
#ifdef SERIALIZATION_ACK
						|| blk->pendingInvAck[block_offset] 
#endif
				  ) 
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
					if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
					{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
						dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						event->operation = NACK;
						event->processingDone = 0;
						event->startCycle = sim_cycle;
						scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
					else	
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] ++;
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}
				} 
				else
					dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
#else		    

				/* See the status of directory */
				if(blk->dir_state[block_offset] == DIR_TRANSITION
#ifdef SERIALIZATION_ACK
						|| blk->pendingInvAck[block_offset] 
#endif
				  )
				{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
					event->isReqL2Trans = 1;
					event->when = sim_cycle + RETRY_TIME;
					flip_counter ++;
					event->flip_flag = 1;
					dir_eventq_insert(event);
					if(collect_stats)
						cp_dir->dir_access ++;
					return 1;
				} 
#endif
				event->reqAtDirTime = sim_cycle;

#ifdef SERIALIZATION_ACK
				if(!Is_ExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid) && Is_Shared(blk->dir_sharer[block_offset], tempID))
				{
					if(isInvCollectTableFull((event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2) || blk->pendingInvAck[block_offset])
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						if(collect_stats)
							cp_dir->dir_access ++;
						return 1;
					}	
					multiple_sharers = blk->dir_sharer[block_offset];
				}
#endif
				if((!IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid)) && (blk->dir_dirty[block_offset] != -1))
					panic("DIR: Block can not be dirty state when it is not exclusive!");

				int isExclusiveOrDirty = Is_ExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid);
				if(collect_stats)
				{
					if(event->operation == WRITE_UPDATE)
						cp_dir->ack_received ++;
					cp_dir->hits ++;
					cp_dir->dhits ++;
					/* Stats regarding number of sharers */
					totalL2WriteReqHits++;
					if(blk->dir_sharer[block_offset][0] == 0 && blk->dir_sharer[block_offset][1] == 0 && blk->dir_sharer[block_offset][2] == 0 && blk->dir_sharer[block_offset][3] == 0)	/* No sharer */
						totalL2WriteReqNoSharer++;
					else if(isExclusiveOrDirty)
					{
						event->isReqL2Inv = 1;
						totalL2WriteReqDirty++;
					}
					else
					{
						event->isReqL2Inv = 1;
						totalL2WriteReqShared++;
					}
				}

#ifdef PREFETCH_OPTI
				if(blk->dir_prefetch[block_offset][tempID]) //prefetch bit reset case 1
					blk->dir_prefetch[block_offset][tempID] = 0;	
#endif

#ifdef SERIALIZATION_ACK
				//if(!IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid))
				if(!isExclusiveOrDirty || (isExclusiveOrDirty && Threadid == tempID))
#else
					if(!IsShared(blk->dir_sharer[block_offset], tempID))
#endif
					{
						if(collect_stats)
						{
							if(blk->ever_dirty == 1)
								involve_2_hop_wb ++;
							if(blk->ever_touch == 1)
								involve_2_hop_touch ++;
							if(event->parent_operation == MISS_WRITE)
							{
								cp_dir->data_access ++;
								involve_2_hops_wm ++;
								WM_Clean ++;
							}
							else
							{
								involve_2_hops_upgrade ++;
								cp_dir->dir_access ++;
							}
						}
						blk->dir_state[block_offset] = DIR_STABLE;
						blk->dir_blk_state[block_offset] = MESI_MODIFIED;
						blk->ptr_cur_event[block_offset] = NULL;
						event->reqAckTime = sim_cycle;
#ifdef PREFETCH_OPTI
						if(blk->dir_prefetch_num[block_offset])
							blk->prefetch_invalidate[block_offset] = 1;	
						if(blk->dir_prefetch_num[block_offset])
							blk->prefetch_modified[block_offset] = 1;	
#endif
						if(event->parent_operation == MISS_WRITE || (multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0 && blk->dir_dirty[block_offset] != -1 && event->operation == WRITE_UPDATE))
						{
							event->operation = ACK_DIR_WRITE;
							packet_size = data_packet_size;
						}
						else
						{
							event->operation = ACK_DIR_WRITEUPDATE;
							packet_size = meta_packet_size;
						}

						data_private_write ++;
						blk->dir_dirty[block_offset] = tempID;
						blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
						event->src1 += event->des1;
						event->des1 = event->src1 - event->des1;
						event->src1 = event->src1 - event->des1;
						event->src2 += event->des2;
						event->des2 = event->src2 - event->des2;
						event->src2 = event->src2 - event->des2;
						
						if(!mystricmp (network_type, "FSOI") || (!mystricmp (network_type, "HYBRID")))
						{
#ifdef EUP_NETWORK
						if(event->early_flag == 1)
							EUP_entry_dellocate((event->src1-MEM_LOC_SHIFT)*mesh_size+event->src2, event->addr>>cache_dl2->set_shift);
						else
						{
#endif
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							event->parent = NULL;
							event->blk_dir = NULL;
							event->pendingInvAck = (multiple_sharers[0] != 0 || multiple_sharers[1] != 0 || multiple_sharers[2] != 0 || multiple_sharers[3] != 0);
							if(event->pendingInvAck == 1 && event->parent_operation == WRITE_UPDATE)
								totalSplitNo ++;
							if(event->pendingInvAck == 1 && event->parent_operation == MISS_WRITE)
								totalSplitWM ++;
							event->arrival_time = sim_cycle - event->arrival_time;
							scheduleThroughNetwork(event, sim_cycle, packet_size, vc);
							dir_eventq_insert(event);
#ifdef EUP_NETWORK
						}
#endif
						}
						else
						{
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							event->parent = NULL;
							event->blk_dir = NULL;
							event->pendingInvAck = (multiple_sharers[0] != 0 || multiple_sharers[1] != 0 || multiple_sharers[2] != 0 || multiple_sharers[3] != 0);
							if(event->pendingInvAck == 1 && event->parent_operation == WRITE_UPDATE)
								totalSplitNo ++;
							if(event->pendingInvAck == 1 && event->parent_operation == MISS_WRITE)
								totalSplitWM ++;
							event->arrival_time = sim_cycle - event->arrival_time;
							scheduleThroughNetwork(event, sim_cycle, packet_size, vc);
							dir_eventq_insert(event);
						}

#ifdef SERIALIZATION_ACK
						if(multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0)
#endif
							return 1;
					}

				/* Invalidate multiple sharers */
				data_shared_write ++;
				if(collect_stats)
				{
					cp_dir->dir_access ++;
					if(event->operation == WRITE_UPDATE)
						involve_4_hops_upgrade ++;
					else
						involve_4_hops_wm ++;
				}
				event->blk_dir = blk;
				if(multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0)
				{
					blk->dir_state[block_offset] = DIR_TRANSITION;
					blk->ptr_cur_event[block_offset] = event;
					event->blk_dir = blk;
					if(blk->dir_dirty[block_offset] != -1 && event->operation == WRITE_UPDATE)
					{
						event->operation = MISS_WRITE;
						event->parent_operation = MISS_WRITE;
					}
				}
				event->childCount = 0;

				if(multiple_sharers[0] == 0 && multiple_sharers[1] == 0 && multiple_sharers[2] == 0 && multiple_sharers[3] == 0)
					multiple_sharers = (blk->dir_sharer[block_offset]);
				IsNeighbor(multiple_sharers, tempID, event->operation, event->addr);

				int multisharers_flag = 0;
				if(IsShared(multiple_sharers, tempID) > 1) 
					multisharers_flag = 1;

				for (Threadid = 0; Threadid < numcontexts; Threadid++)
				{
					if((((multiple_sharers[Threadid/64]) & ((unsigned long long int)1 << (Threadid%64))) == ((unsigned long long int)1 << (Threadid%64))) && (Threadid != tempID))
					{
						struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);

						if(new_event == NULL) 	panic("Out of Virtual Memory");
						new_event->req_time = 0;
						new_event->conf_bit = -1;
						new_event->input_buffer_full = 0;
						new_event->pending_flag = 0;
						new_event->op = event->op;
						new_event->isPrefetch = 0;
						new_event->L2miss_flag = event->L2miss_flag;
#ifdef LOCK_CACHE_REGISTER
						if(event->store_cond == MY_STL_C && multisharers_flag) //FIXME: need to check if the value is boolean or not
						{
							new_event->operation = INV_MSG_UPDATE;
							new_event->conf_bit = event->childCount;
						}
						else
#endif
							new_event->operation = INV_MSG_WRITE;
						new_event->isExclusiveOrDirty = isExclusiveOrDirty;
						new_event->src1 = currentThreadId/mesh_size+MEM_LOC_SHIFT;
						new_event->src2 = currentThreadId%mesh_size;
						new_event->des1 = Threadid/mesh_size+MEM_LOC_SHIFT;
						new_event->des2 = Threadid%mesh_size;
						new_event->processingDone = 0;
						new_event->startCycle = sim_cycle;
						new_event->parent = event;
						new_event->tempID = tempID;
						new_event->resend = 0;
						new_event->blk_dir = blk;
						new_event->cp = event->cp;
						new_event->addr = event->addr;
						new_event->vp = NULL;
						new_event->nbytes = event->cp->bsize;
						new_event->udata = NULL;
						new_event->cmd = Write;
						new_event->rs = event->rs;
						new_event->started = sim_cycle;
						new_event->store_cond = event->store_cond;
						new_event->spec_mode = 0;
							event->childCount++;
						if(!mystricmp(network_type, "FSOI") || (!mystricmp(network_type, "HYBRID")))
						{
#ifdef INV_ACK_CON
						int thread_id;
						if(Is_Shared(multiple_sharers, tempID) > 1) 
						{
							if((new_event->src1*mesh_size+new_event->src2 != new_event->des1*mesh_size+new_event->des2) && ((!mystricmp(network_type, "FSOI")) || (!mystricmp(network_type, "HYBRID") && ((abs(new_event->src1 - new_event->des1) + abs(new_event->src2 - new_event->des2)) > 1))))
							{
								new_event->data_reply = 0;
								struct DIRECTORY_EVENT *new_event2 = allocate_event(event->isSyncAccess);
								if(new_event2 == NULL)       panic("Out of Virtual Memory");
								if(collect_stats)
									cp_dir->dir_access ++;
								new_event2->input_buffer_full = 0;
								new_event2->req_time = 0;
								new_event2->conf_bit = -1;
								new_event2->pending_flag = 0;
								new_event2->op = event->op;
								new_event2->isPrefetch = 0;
#ifdef LOCK_CACHE_REGISTER
								if(event->store_cond == MY_STL_C && multisharers_flag) //FIXME: need to check if the value if boolean or not
										new_event2->operation = ACK_MSG_UPDATE;
								else
#endif
								new_event2->operation = ACK_MSG_WRITEUPDATE;
								new_event2->store_cond = event->store_cond;
								new_event2->src1 = new_event->des1;
								new_event2->src2 = new_event->des2;
								new_event2->des1 = new_event->src1;
								new_event2->des2 = new_event->src2;
								new_event2->processingDone = 0;
								new_event2->startCycle = sim_cycle;
								new_event2->parent = event;
								new_event2->tempID = event->tempID;
								new_event2->resend = 0;
								new_event2->blk_dir = blk;
								new_event2->cp = event->cp;
								new_event2->addr = event->addr;
								new_event2->vp = NULL;
								new_event2->nbytes = event->cp->bsize;
								new_event2->udata = NULL;
								new_event2->cmd = Write;
								new_event2->rs = event->rs;
								new_event2->started = sim_cycle;
								new_event2->spec_mode = 0;
								new_event2->L2miss_flag = event->L2miss_flag;
								new_event2->data_reply = 0;
								popnetMsgNo ++;
								new_event2->popnetMsgNo = popnetMsgNo;
								new_event2->when = WAIT_TIME;
								dir_eventq_insert(new_event2);
							}
							else	
								new_event->data_reply = 1;
						}
						else	
							new_event->data_reply = 1;
#else
						new_event->data_reply = 1;
#endif
						}
						else 
							new_event->data_reply = 1;
						event->dirty_flag = 1;
						/*go to network*/
						scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);

						dir_eventq_insert(new_event);
					}
				}
				if(event->childCount == 1 && event->parent_operation == MISS_WRITE)
				{
					WM_EM ++;
					blk->exclusive_time[block_offset] = sim_cycle;
					event->originalChildCount = event->childCount;
				}
				else if(event->parent_operation == MISS_WRITE)
					WM_S ++;
				sharer_distr[event->childCount] ++;
				IsNeighbor_sharer(multiple_sharers, tempID);
#ifdef SERIALIZATION_ACK
				//if(!isExclusiveOrDirty && event->childCount)
				if(event->childCount >= 1 && !isExclusiveOrDirty)
				{
					int final_ack_conf = 0;
					if(event->operation == ACK_DIR_WRITEUPDATE)
						final_ack_conf = 1;
					else	
						final_ack_conf = 0;

					if(event->operation == ACK_DIR_WRITE)
						write_shared_used_conf ++;
					newInvCollect((event->src1-MEM_LOC_SHIFT)*mesh_size+event->src2, event->addr, event->childCount, tempID, final_ack_conf);
					blk->pendingInvAck[block_offset] = 1;
					if(!mystricmp (network_type, "MESH"))
					{
						if(event->operation == ACK_DIR_WRITEUPDATE)
						{
							totalUpgradesUsable++;
							if(thecontexts[tempID]->m_shSQNum > m_shSQSize-3)
								totalUpgradesBeneficial++;
						}
					}
				}
#endif
				return 1;
			}
			else
			{   /* L2 Miss */
				if(event->operation == WRITE_UPDATE)
					printf("here\n");
				matchnum = MSHR_block_check(cache_dl2->mshr, event->addr, cache_dl2->set_shift);
				event->L2miss_flag = 1;
				event->L2miss_stated = sim_cycle;
				if(event->isSyncAccess)
					Sync_L2_miss ++;
				if(!matchnum)
				{ /* No match in L2 MSHR */
#ifdef DIR_FIFO_FULL
					if(!cache_dl2->mshr->freeEntries)
					{/* We can not proceed with this even, since the directory is in transition. We wait till directory is in stable state. */
						if(!last_L2_mshr_full)
						{
							L2_mshr_full ++;
							last_L2_mshr_full = sim_cycle;
						}
#ifdef TSHR
					if(tshr_fifo_num[event->des1*mesh_size+event->des2] >= tshr_fifo_size)
#else
						if(dir_fifo_full[event->des1*mesh_size+event->des2] >= dir_fifo_size)
#endif
						{ /* indicate that dir fifo is full, so send the NACK to the requester to resend later */
							dir_fifo_full[event->des1*mesh_size+event->des2] = dir_fifo_full[event->des1*mesh_size+event->des2] - 4;
							event->src1 += event->des1;
							event->des1 = event->src1 - event->des1;
							event->src1 = event->src1 - event->des1;
							event->src2 += event->des2;
							event->des2 = event->src2 - event->des2;
							event->src2 = event->src2 - event->des2;
							event->operation = NACK;
							event->processingDone = 0;
							event->startCycle = sim_cycle;
							scheduleThroughNetwork(event, sim_cycle, meta_packet_size, vc);
							dir_eventq_insert(event);
							return 1;
						}
						else	
						{
							dir_fifo_full[event->des1*mesh_size+event->des2] ++;
							event->when = sim_cycle + RETRY_TIME;
							flip_counter ++;
							event->flip_flag = 1;
							dir_eventq_insert(event);
							return 1;
						}
					} 
					else
					{
						dir_fifo_full[event->des1*mesh_size+event->des2] = 0;
						if(last_L2_mshr_full)
						{
							Stall_L2_mshr += sim_cycle - last_L2_mshr_full;
							last_L2_mshr_full = 0;
						}
					}
#else
					if(!cache_dl2->mshr->freeEntries)
					{
						event->when = sim_cycle + RETRY_TIME;
						flip_counter ++;
						event->flip_flag = 1;
						dir_eventq_insert(event);
						return 1;
					}	
#endif
					data_private_write ++;
					if(collect_stats)
					{
						cache_dl2->misses ++;
						cache_dl2->dmisses ++;
						involve_2_hops_miss ++;
					}
					event->reqAtDirTime = sim_cycle;
					event->mshr_time = sim_cycle;
					struct DIRECTORY_EVENT *new_event;
					new_event = allocate_new_event(event);
					new_event->operation = MEM_READ_REQ;
					new_event->cmd = Read;
					event->when = sim_cycle + WAIT_TIME;
					event->operation = WAIT_MEM_READ;
					scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, vc);
					dir_eventq_insert(new_event);
					if(event->operation != WRITE_UPDATE)
						WM_Miss ++;
					event->startCycle = sim_cycle;
					event_created = event;
					MSHRLookup(cache_dl2->mshr, event->addr, WAIT_TIME, 0, NULL);
					return 1;
				}
				else
				{  /* match entry in L2 MSHR, then insert this event into eventlist appending to mshr entry */
					event->isReqL2SecMiss = 1;
					if(collect_stats)
					{
						cache_dl2->in_mshr ++;
						cache_dl2->din_mshr ++;
					}
					event_list_insert(event, matchnum);
					return 1;	
				}
			}

		case UPDATE_DIR		:
#ifdef UPDATE_HINT
			dcache2_access++;
			thecontexts[event->des1*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* hit */
				if(collect_stats)
					cp_dir->data_access ++;

				blk->ever_touch = 1;
				blk->dir_sharer[block_offset][tempID/64] &= ~(((unsigned long long)1<<(tempID%64)));
				free_event(event);
				return 1;
			}
			else
			{/* miss */
				//panic("DIR: We mainting cache inclusion accross cache hierarchy. There cannot be a miss for update");
				free_event(event);
				return 1;
			}
#endif
#ifdef SILENT_DROP
			panic("This Update_dir part should never be entered!\n");
#endif
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* hit */
				blk->ever_touch = 1;	
				if(collect_stats)
				{
					cp_dir->hits ++;
					cp_dir->ack_received ++;
					cp_dir->dir_access ++;
				}
				/* See the status of directory */
				if(blk->dir_state[block_offset] == DIR_TRANSITION)
				{/* Directory must be waiting for another messages to complete i.e. INV. Since we see a WB for the same line, the dir must be in dirty (modified) state. We assume that the home l1 is going to drop the original INV message. We complete the request as if we received the INV ACK. */
					if(!blk->ptr_cur_event[block_offset]) panic("DIR (WB): parent event missing");
					if(blk->ptr_cur_event[block_offset]->operation != MISS_READ && blk->ptr_cur_event[block_offset]->operation != MISS_WRITE && blk->ptr_cur_event[block_offset]->operation != WRITE_UPDATE && blk->ptr_cur_event[block_offset]->operation != WAIT_MEM_READ)
						panic("DIR (WB): the original message has invalid operation");
					//blk->ptr_cur_event->childCount--;
					if((blk->ptr_cur_event[block_offset])->operation == WAIT_MEM_READ)
					{
						(blk->ptr_cur_event[block_offset])->individual_childCount[block_offset] --;
						blk->ptr_cur_event[block_offset]->childCount--;
					}
					else if(block_offset == blockoffset(blk->ptr_cur_event[block_offset]->addr))
						blk->ptr_cur_event[block_offset]->childCount--;

					if((blk->ptr_cur_event[block_offset])->operation == WAIT_MEM_READ && (blk->ptr_cur_event[block_offset])->individual_childCount[block_offset] == 0)	
					{  
						(blk->ptr_cur_event[block_offset])->sharer_num --;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_dirty[block_offset] = -1;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][0] = 0;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][1] = 0;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][2] = 0;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][3] = 0;

						if((blk->ptr_cur_event[block_offset])->sharer_num != 0)
						{
							free_event(event);
							return 1;
						}
					}

					if(blk->ptr_cur_event[block_offset]->childCount == 0)
					{
						int old_boffset = block_offset;
						block_offset = blockoffset(blk->ptr_cur_event[old_boffset]->addr);
						counter_t startCycle = sim_cycle;
						tempID = blk->ptr_cur_event[old_boffset]->tempID;
						blk->dir_state[block_offset] = DIR_STABLE;
						blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);

						if(blk->ptr_cur_event[old_boffset]->operation == WAIT_MEM_READ)
						{
							/* L2 write back latency */	
							if(!blk->Iswb_to_mem && (blk->status & CACHE_BLK_DIRTY) && (blk->status & CACHE_BLK_VALID))
							{
								lat = cp_dir->blk_access_fn(Write, CACHE_BADDR(cp_dir,event->addr), cp_dir->bsize, NULL, sim_cycle, event->tempID);
								if(collect_stats)
									cache_dl2->writebacks ++;
							}

							int i=0, m=0, iteration = 1;
							for(i=0; i<(cache_dl2->set_shift - cache_dl1[0]->set_shift); i++)
								iteration = iteration * 2; 
							for(m=0; m<iteration; m++)
								blk->dir_state[m] = DIR_STABLE;

							tag_dir = CACHE_TAG(cp_dir, blk->ptr_cur_event[old_boffset]->addr);
							blk->tagid.tag = tag_dir;
							blk->tagid.threadid =  blk->ptr_cur_event[old_boffset]->tempID;	
							blk->addr = blk->ptr_cur_event[old_boffset]->addr;
							blk->status = CACHE_BLK_VALID;
							blk->Iswb_to_mem = 0;
							/* jing: Here is a problem: copy data out of cache block, if block exists */
							/* do something here */
							if (blk->way_prev && cp_dir->policy == LRU)
								update_way_list(&cp_dir->sets[set_dir], blk, Head);
							cp_dir->last_tagsetid.tag = 0;
							cp_dir->last_tagsetid.threadid = blk->ptr_cur_event[old_boffset]->tempID;
							cp_dir->last_blk = blk;
							/* get user block data, if requested and it exists */
							if (blk->ptr_cur_event[old_boffset]->udata)
								*(blk->ptr_cur_event[old_boffset]->udata) = blk->user_data;
							blk->ready = sim_cycle;
							if (cp_dir->hsize)
								link_htab_ent(cp_dir, &cp_dir->sets[set_dir], blk);

							/* remove the miss entry in L2 MSHR*/ 
							update_L2_mshrentry(blk->ptr_cur_event[old_boffset]->addr, sim_cycle, blk->ptr_cur_event[old_boffset]->mshr_time);

							if(blk->ptr_cur_event[old_boffset]->parent_operation == L2_PREFETCH)
							{	
								blk->dir_sharer[block_offset][0] = 0;
								blk->dir_sharer[block_offset][1] = 0;
								blk->dir_sharer[block_offset][2] = 0;
								blk->dir_sharer[block_offset][3] = 0;
								blk->dir_dirty[block_offset] = -1;
								free_event(event);
								free_event(blk->ptr_cur_event[old_boffset]);
								return 1;
							}
							if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_IL1)
							{
								packet_size = data_packet_size;
								blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_IL1;
								blk->dir_blk_state[block_offset] = MESI_SHARED;
								blk->dir_dirty[block_offset] = -1;
								blk->dir_sharer[block_offset][0] = 0;
								blk->dir_sharer[block_offset][1] = 0;
								blk->dir_sharer[block_offset][2] = 0;
								blk->dir_sharer[block_offset][3] = 0;
								startCycle = sim_cycle;
							}
							else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_READ)
							{
								packet_size = data_packet_size;
								blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_READ_EXCLUSIVE;
								blk->dir_dirty[block_offset] = -1;
#ifdef LOCK_CACHE_REGISTER
								/* update the lock register if necessary */
								if(blk->ptr_cur_event[old_boffset]->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
									LockRegesterCheck(blk->ptr_cur_event[old_boffset]);
#endif				
								if (!md_text_addr(blk->ptr_cur_event[old_boffset]->addr, blk->ptr_cur_event[old_boffset]->tempID))
								{
									blk->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
									blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
								}
								else
								{
									blk->dir_blk_state[block_offset] = MESI_SHARED;
									blk->dir_sharer[block_offset][0] = 0;
									blk->dir_sharer[block_offset][1] = 0;
									blk->dir_sharer[block_offset][2] = 0;
									blk->dir_sharer[block_offset][3] = 0;
								}
							}
							else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_WRITE)
							{
								packet_size = data_packet_size;
								blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITE;
								blk->dir_blk_state[block_offset] = MESI_MODIFIED;
								blk->dir_dirty[block_offset] = tempID;
								blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
							}
							else
							{
								packet_size = meta_packet_size;
								blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITEUPDATE;
								blk->dir_blk_state[block_offset] = MESI_MODIFIED;
								blk->dir_dirty[block_offset] = tempID;
								blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
							}
						}
						else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_READ)
						{
							packet_size = data_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_READ_EXCLUSIVE;
							blk->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
							blk->dir_dirty[block_offset] = -1;
#ifdef LOCK_CACHE_REGISTER
							/* update the lock register if necessary */
							if(blk->ptr_cur_event[old_boffset]->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
								LockRegesterCheck(blk->ptr_cur_event[old_boffset]);
#endif				
						}
						else if(blk->ptr_cur_event[old_boffset]->parent_operation == WRITE_UPDATE)
						{
							packet_size = meta_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITEUPDATE;
							blk->dir_blk_state[block_offset] = MESI_MODIFIED;
							blk->dir_dirty[block_offset] = tempID;
						}
						else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_WRITE)
						{
							packet_size = data_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITE;
							blk->dir_blk_state[block_offset] = MESI_MODIFIED;
							blk->dir_dirty[block_offset] = tempID;
						}
						else 
							panic("DIR UD: all state options exhausted.");

						blk->ptr_cur_event[old_boffset]->startCycle = startCycle;
						blk->ptr_cur_event[old_boffset]->src1 = blk->ptr_cur_event[old_boffset]->des1;
						blk->ptr_cur_event[old_boffset]->src2 = blk->ptr_cur_event[old_boffset]->des2;
						blk->ptr_cur_event[old_boffset]->des1 = tempID/mesh_size+MEM_LOC_SHIFT;
						blk->ptr_cur_event[old_boffset]->des2 = tempID%mesh_size;
						blk->ptr_cur_event[old_boffset]->processingDone = 0;
						scheduleThroughNetwork(blk->ptr_cur_event[old_boffset], blk->ptr_cur_event[old_boffset]->startCycle, packet_size, vc);
						dir_eventq_insert(blk->ptr_cur_event[old_boffset]);
						blk->ptr_cur_event[old_boffset] = NULL;
						free_event(event);
						return 1;
					}
				}

				blk->dir_sharer[block_offset][tempID/64] &= ~((unsigned long long int)1<<(tempID%64));
				free_event(event);
				return 1;
			}
			else
			{/* miss */
				panic("DIR: We mainting cache inclusion accross cache hierarchy. There cannot be a miss for Update Dir");
				return 1;
			}

		case WRITE_BACK		:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			event->parent_operation = event->operation;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
			{/* hit */
				if(collect_stats)
					cp_dir->data_access ++;

				blk->status |= CACHE_BLK_DIRTY;
				blk->ever_dirty = 1;
				blk->ever_touch = 1;
#ifdef NON_SILENT_DROP
				/* See the status of directory */
				if(blk->dir_state[block_offset] == DIR_TRANSITION)
				{/* Directory must be waiting for another messages to complete i.e. INV. Since we see a WB for the same line, the dir must be in dirty (modified) state. We assume that the home l1 is going to drop the original INV message. We complete the request as if we received the INV ACK. */
					counter_t startCycle = sim_cycle;
					if(blk->dir_blk_state[block_offset] == MESI_SHARED || blk->dir_blk_state[block_offset] == MESI_INVALID)	panic("DIR: The directory is not in dirty state");
					if(!blk->ptr_cur_event[block_offset]) panic("DIR (WB): parent event missing");
					if(blk->ptr_cur_event[block_offset]->operation != MISS_READ && blk->ptr_cur_event[block_offset]->operation != MISS_WRITE && blk->ptr_cur_event[block_offset]->operation != WRITE_UPDATE && blk->ptr_cur_event[block_offset]->operation != WAIT_MEM_READ)
						panic("DIR (WB): the original message has invalid operation");


					if(blk->ptr_cur_event[block_offset]->operation == WAIT_MEM_READ)
					{

						(blk->ptr_cur_event[block_offset])->individual_childCount[block_offset] --;
						(blk->ptr_cur_event[block_offset])->childCount --;
						if((blk->ptr_cur_event[block_offset])->individual_childCount[block_offset] == 0)	
						{  
							(blk->ptr_cur_event[block_offset])->sharer_num --;
							((blk->ptr_cur_event[block_offset])->blk_dir)->dir_dirty[block_offset] = -1;
							((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][0] = 0;
							((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][1] = 0;
							((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][2] = 0;
							((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][3] = 0;

							if((blk->ptr_cur_event[block_offset])->sharer_num != 0)
							{
								free_event(event);
								return 1;
							}
						}
					}
					if(block_offset != blockoffset(blk->ptr_cur_event[block_offset]->addr) && blk->ptr_cur_event[block_offset]->operation != WAIT_MEM_READ)
					{
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_dirty[block_offset] = -1;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][0] = 0;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][1] = 0;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][2] = 0;
						((blk->ptr_cur_event[block_offset])->blk_dir)->dir_sharer[block_offset][3] = 0;
						free_event(event);
						return 1;
					}

					tempID = blk->ptr_cur_event[block_offset]->tempID;
					int old_boffset = block_offset;
					block_offset = blockoffset(blk->ptr_cur_event[old_boffset]->addr);
					blk->dir_state[block_offset] = DIR_STABLE;
					blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);

					if(blk->ptr_cur_event[old_boffset]->operation == WAIT_MEM_READ)
					{
						/* L2 write back latency */
						if(!blk->Iswb_to_mem)
						{
							lat = cp_dir->blk_access_fn(Write, CACHE_BADDR(cp_dir,event->addr), cp_dir->bsize, NULL, sim_cycle, event->tempID);
							if(collect_stats)
								cache_dl2->writebacks ++;
						}
						tag_dir = CACHE_TAG(cp_dir, blk->ptr_cur_event[old_boffset]->addr);
						int i=0, m=0, iteration = 1;
						for(i=0; i<(cache_dl2->set_shift - cache_dl1[0]->set_shift); i++)
							iteration = iteration * 2; 
						for(m=0; m<iteration; m++)
							blk->dir_state[m] = DIR_STABLE;
						blk->tagid.tag = tag_dir;
						blk->tagid.threadid =  blk->ptr_cur_event[old_boffset]->tempID;	
						blk->addr = blk->ptr_cur_event[old_boffset]->addr;
						blk->status = CACHE_BLK_VALID;
						blk->Iswb_to_mem = 0;
						/* jing: Here is a problem: copy data out of cache block, if block exists */
						/* do something here */
						if (blk->way_prev && cp_dir->policy == LRU)
							update_way_list(&cp_dir->sets[set_dir], blk, Head);
						cp_dir->last_tagsetid.tag = 0;
						cp_dir->last_tagsetid.threadid = blk->ptr_cur_event[old_boffset]->tempID;
						cp_dir->last_blk = blk;
						/* get user block data, if requested and it exists */
						if (blk->ptr_cur_event[old_boffset]->udata)
							*(blk->ptr_cur_event[old_boffset]->udata) = blk->user_data;
						blk->ready = sim_cycle;
						if (cp_dir->hsize)
							link_htab_ent(cp_dir, &cp_dir->sets[set_dir], blk);

						/* remove the miss entry in L2 MSHR*/ 
						update_L2_mshrentry(blk->ptr_cur_event[old_boffset]->addr, sim_cycle, blk->ptr_cur_event[old_boffset]->mshr_time);

						if(blk->ptr_cur_event[old_boffset]->parent_operation == L2_PREFETCH)
						{	
							blk->dir_sharer[block_offset][0] = 0;
							blk->dir_sharer[block_offset][1] = 0;
							blk->dir_sharer[block_offset][2] = 0;
							blk->dir_sharer[block_offset][3] = 0;
							blk->dir_dirty[block_offset] = -1;
							free_event(event);
							free_event(blk->ptr_cur_event[old_boffset]);
							return 1;
						}

						if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_IL1)
						{
							packet_size = data_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_IL1;
							blk->dir_dirty[block_offset] = -1;
							blk->dir_blk_state[block_offset] = MESI_SHARED;
							blk->dir_sharer[block_offset][0] = 0;
							blk->dir_sharer[block_offset][1] = 0;
							blk->dir_sharer[block_offset][2] = 0;
							blk->dir_sharer[block_offset][3] = 0;
							startCycle = sim_cycle;
						}
						else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_READ)
						{
							packet_size = data_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_READ_EXCLUSIVE;
							blk->dir_dirty[block_offset] = -1;
#ifdef LOCK_CACHE_REGISTER
							/* update the lock register if necessary */
							if(blk->ptr_cur_event[old_boffset]->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
								LockRegesterCheck(blk->ptr_cur_event[old_boffset]);
#endif				
							if (!md_text_addr(blk->ptr_cur_event[old_boffset]->addr, blk->ptr_cur_event[old_boffset]->tempID))
							{
								blk->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
								blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
							}
							else
							{
								blk->dir_blk_state[block_offset] = MESI_SHARED;
								blk->dir_sharer[block_offset][0] = 0;
								blk->dir_sharer[block_offset][1] = 0;
								blk->dir_sharer[block_offset][2] = 0;
								blk->dir_sharer[block_offset][3] = 0;
							}
						}
						else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_WRITE)
						{
							packet_size = data_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITE;
							blk->dir_dirty[block_offset] = tempID;
							blk->dir_blk_state[block_offset] = MESI_MODIFIED;
							blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
						}
						else
						{
							packet_size = meta_packet_size;
							blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITEUPDATE;
							blk->dir_dirty[block_offset] = tempID;
							blk->dir_blk_state[block_offset] = MESI_MODIFIED;
							blk->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
						}
					}
					else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_READ)
					{
						packet_size = data_packet_size;
						blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_READ_EXCLUSIVE;
						blk->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
						blk->dir_dirty[block_offset] = -1;
#ifdef LOCK_CACHE_REGISTER
						/* update the lock register if necessary */
						if(blk->ptr_cur_event[old_boffset]->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
							LockRegesterCheck(blk->ptr_cur_event[old_boffset]);
#endif				
					}
					else if(blk->ptr_cur_event[old_boffset]->parent_operation == WRITE_UPDATE)
					{
						packet_size = meta_packet_size;
						blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITEUPDATE;
						blk->dir_blk_state[block_offset] = MESI_MODIFIED;
						blk->dir_dirty[block_offset] = tempID;
					}
					else if(blk->ptr_cur_event[old_boffset]->parent_operation == MISS_WRITE)
					{
						packet_size = data_packet_size;
						blk->ptr_cur_event[old_boffset]->operation = ACK_DIR_WRITE;
						blk->dir_blk_state[block_offset] = MESI_MODIFIED;
						blk->dir_dirty[block_offset] = tempID;
					}
					else 
						panic("DIR WB: all state options exhausted.");

					blk->ptr_cur_event[old_boffset]->startCycle = startCycle;
					blk->ptr_cur_event[old_boffset]->src1 = blk->ptr_cur_event[old_boffset]->des1;
					blk->ptr_cur_event[old_boffset]->src2 = blk->ptr_cur_event[old_boffset]->des2;
					blk->ptr_cur_event[old_boffset]->des1 = tempID/mesh_size+MEM_LOC_SHIFT;
					blk->ptr_cur_event[old_boffset]->des2 = tempID%mesh_size;
					blk->ptr_cur_event[old_boffset]->processingDone = 0;
					scheduleThroughNetwork(blk->ptr_cur_event[old_boffset], blk->ptr_cur_event[old_boffset]->startCycle, packet_size, vc);
					dir_eventq_insert(blk->ptr_cur_event[old_boffset]);
					blk->ptr_cur_event[old_boffset] = NULL;
					free_event(event);
					return 1;
				} 
#endif
#ifdef PREFETCH_OPTI
				if(blk->dir_prefetch[block_offset][tempID]) //prefetch bit reset case 2
				{
					blk->dir_prefetch[block_offset][tempID] = 0;	
					blk->dir_prefetch_num[block_offset] --;
					if(blk->dir_prefetch_num[block_offset] < 0)
						panic("prefetch number does not match!\n");
#ifdef SILENT_DROP
					panic("prefetch can not be write back!\n");  //prefetch the data in exclusive state, see L1_PREFETCH
#endif
				}
				if(blk->dir_prefetch_num[block_offset]) //other node read the data in exclusive state and modified later, so when the data is evicted from the L1 cache, it will set the prefetch_modified bit into 1
					blk->prefetch_modified[block_offset] = 1;	
#endif
				/* Directory in stable state: just complete the event in stable state. */
				if(!mystricmp (network_type, "FSOI") || (!mystricmp (network_type, "HYBRID")))
				{
#ifdef EUP_NETWORK
				if(blk->dir_blk_state[block_offset] == MESI_SHARED)
					blk->dir_sharer[block_offset][tempID/64] &= ~((unsigned long long int)1<<(event->tempID%64));
				else
				{
					blk->dir_sharer[block_offset][0] = 0;
					blk->dir_sharer[block_offset][1] = 0;
					blk->dir_sharer[block_offset][2] = 0;
					blk->dir_sharer[block_offset][3] = 0;
					blk->dir_dirty[block_offset] = -1;
				}
#else
				if(blk->dir_blk_state[block_offset] == MESI_SHARED)
					panic("DIR: L2 cache block not in modified state during WB");
				blk->dir_sharer[block_offset][0] = 0;
				blk->dir_sharer[block_offset][1] = 0;
				blk->dir_sharer[block_offset][2] = 0;
				blk->dir_sharer[block_offset][3] = 0;
				blk->dir_dirty[block_offset] = -1;
#endif
				}
				else
				{
					if(blk->dir_blk_state[block_offset] == MESI_SHARED)
						panic("DIR: L2 cache block not in modified state during WB");
					blk->dir_sharer[block_offset][0] = 0;
					blk->dir_sharer[block_offset][1] = 0;
					blk->dir_sharer[block_offset][2] = 0;
					blk->dir_sharer[block_offset][3] = 0;
					blk->dir_dirty[block_offset] = -1;
				}
				free_event(event);
				return 1;
			}
			else
			{/* miss */
				panic("DIR: We mainting cache inclusion accross cache hierarchy. There cannot be a miss for WB");
				return 1;
			}

		case INV_MSG_READ	:	
			dcache_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache_access++;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			count = (event->des1-MEM_LOC_SHIFT) * mesh_size + event->des2;			
			cp1 = cache_dl1[count];
			tag = CACHE_TAG(cp1,addr);
			set = CACHE_SET(cp1,addr);
			if(collect_stats)
				cp1->Invalid_Read_received ++;
#ifdef SILENT_DROP
			packet_size = meta_packet_size;
			event->operation = ACK_MSG_READUPDATE;
#endif
#ifdef EXCLUSIVE_OPT
			event->exclusive_after_wb = 0;
#endif
			event->l1LineUseStatus = 0;
			for (blk1 = cp1->sets[set].way_head; blk1; blk1 =blk1->way_next)
			{
				if ((blk1->tagid.tag == tag) && (blk1->status & CACHE_BLK_VALID))
				{
					if(blk1->state == MESI_MODIFIED)
					{  /* blk state is modified, then the blk data should be carried back into L2 cache at home node */
						packet_size = data_packet_size;
						event->operation = ACK_MSG_READ;
						if(collect_stats)
							cp1->wb_coherence_r ++;
						if(event->data_reply == 0)
							total_exclusive_cross ++;
					}
					else if(blk1->state == MESI_EXCLUSIVE)
					{
						packet_size = meta_packet_size;
						event->operation = ACK_MSG_READUPDATE;
					}
					//else
					//	panic("DIR: Can not be MESI_shared ");

					/* blk state changed from exclusive/modified to shared state */
					if((blk1->status & CACHE_BLK_VALID) && blk1->state == MESI_EXCLUSIVE)
						totalL1LineExlInv++;

					blk1->state = MESI_SHARED;
					blk1->status = CACHE_BLK_VALID; 
					if(collect_stats)
						cp1->Invalid_Read_received_hits ++;
					event->l1LineUseStatus = !blk1->blkImdtOp;
					break;
				}
			}
#ifdef NON_SILENT_DROP
			if(!blk1)	/* if there is a miss in L1, we just drop this request. This case can only be due to write back event intiated before. */
			{
				free_event(event);
				return 1;
			}
#endif
#ifdef EXCLUSIVE_OPT 
			if(!blk1)
				event->exclusive_after_wb = 1;
#endif
			if(!mystricmp (network_type, "FSOI") || !mystricmp(network_type, "HYBRID"))
			{
#ifdef INV_ACK_CON
			if(event->data_reply == 0 && (event->src1*mesh_size+event->src2 != event->des1*mesh_size+event->des2))
			{
				pending_invalidation[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2] --;
				if(pending_invalidation[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2] == 0)
				{
					pending_invalid_cycles = sim_cycle - pending_invalid_start_cycles[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2];
				}
				free_event(event);
				return 1;	
			}
#endif
			}
			event->src1 += event->des1;
			event->des1 = event->src1 - event->des1;
			event->src1 = event->src1 - event->des1;
			event->src2 += event->des2;
			event->des2 = event->src2 - event->des2;
			event->src2 = event->src2 - event->des2;
			event->processingDone = 0;
			event->startCycle = sim_cycle;
			/* schedule a network access */
			scheduleThroughNetwork(event, sim_cycle, packet_size, vc);

			dir_eventq_insert(event);
			return 1;

		case INV_MSG_WRITE	:	
			dcache_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache_access++;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			count = (event->des1-MEM_LOC_SHIFT) * mesh_size + event->des2;
			cp1 = cache_dl1[count];
			tag = CACHE_TAG(cp1,addr);
			set = CACHE_SET(cp1,addr);
			sendLSQInv(addr, count);
			if(collect_stats)
				cp1->Invalid_write_received ++;
			int shared_flag = 0;
#ifdef SILENT_DROP
			packet_size = meta_packet_size;
			event->operation = ACK_MSG_WRITEUPDATE;
#endif
			event->l1LineUseStatus = 0;
			for (blk1 = cp1->sets[set].way_head; blk1; blk1 =blk1->way_next)
			{
				if ((blk1->tagid.tag == tag) && (blk1->status & CACHE_BLK_VALID))
				{
					if(blk1->state == MESI_MODIFIED)
					{  /* blk state is modified, then the blk data should be carried back into L2 cache at home node */
						packet_size = data_packet_size;
						event->operation = ACK_MSG_WRITE;
						if(collect_stats)
							cp1->wb_coherence_w ++;
					}
					else 
					{
						packet_size = meta_packet_size;
						event->operation = ACK_MSG_WRITEUPDATE;
						shared_flag = 1;
						if(collect_stats && blk1->state == MESI_SHARED)
							cp1->s_to_i ++;
						else if(collect_stats && blk1->state == MESI_EXCLUSIVE)
							cp1->e_to_i ++;
					}
					/* blk state changed from exclusive/modified to shared state */
					if(collect_stats)
						cp1->Invalid_write_received_hits ++;

					if((blk1->status & CACHE_BLK_VALID) && blk1->state == MESI_EXCLUSIVE)
						totalL1LineExlInv++;
					
					int m = 0;
					for(m=0;m<8;m++)
						if(blk1->WordUseFlag[m])
							blk1->WordCount ++;
					if(event->parent->operation == WAIT_MEM_READ)
						stats_do(blk1->addr&(~(cache_dl1[0]->bsize-1)), blk1->ReadCount, blk1->WriteCount, blk1->WordCount, blk1->Type, count);
					else
						stats_do(blk1->addr&(~(cache_dl1[0]->bsize-1)), blk1->ReadCount, blk1->WriteCount, blk1->WordCount, SHARED_READ_WRITE, count);
					blk1->ReadCount = 0;	
					blk1->WriteCount = 0;	
					blk1->WordCount = 0;	
					for(m=0;m<8;m++)
						blk1->WordUseFlag[m] = 0;

					blk1->state = MESI_INVALID;
					blk1->status = 0;
					event->l1LineUseStatus = !blk1->blkImdtOp;
					if(blk1->blkAlocReason == LINE_PREFETCH)
					{
						if(blk1->blkImdtOp)
							totalL1LinePrefUse++;
						else
							totalL1LinePrefNoUse++;
					}
					else if(blk1->blkAlocReason == LINE_READ)
					{
						if(blk1->blkImdtOp)
							totalL1LineReadUse++;
						else
							totalL1LineReadNoUse++;
					}
					else if (blk1->blkAlocReason == LINE_WRITE)
					{
						if(blk1->blkImdtOp)
							totalL1LineWriteUse++;
						else
							totalL1LineWriteNoUse++;
					}
					break;
				}
			}
#ifdef NON_SILENT_DROP
			if(!blk1)	/* if there is a miss in L1, we just drop this request. This case can only be due to write back event intiated before. */
			{
				int match;
				if((match=MSHR_block_check(cp1->mshr, event->addr, cp1->set_shift)))
				{
					if(cp1->mshr->mshrEntry[match-1].event->operation != WRITE_UPDATE)
					{
						if(cp1->mshr->mshrEntry[match-1].event->operation == NACK && (cp1->mshr->mshrEntry[match-1].event->parent_operation == WRITE_UPDATE))
						{
							packet_size = meta_packet_size;
							event->operation = ACK_MSG_WRITEUPDATE;
						}
						else
						{
							free_event(event);
							return 1;
						}
					}
					else
					{
						packet_size = meta_packet_size;
						event->operation = ACK_MSG_WRITEUPDATE;
					}
				}
				else
				{
					free_event(event);
					return 1;
				}
			}
#endif
			if(!mystricmp (network_type, "FSOI") || !mystricmp(network_type, "HYBRID"))
			{
#ifdef INV_ACK_CON
			if(event->data_reply == 0 && (event->src1*mesh_size+event->src2 != event->des1*mesh_size+event->des2))
			{
				pending_invalidation[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2] --;
				if(pending_invalidation[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2] == 0)
				{
					pending_invalid_cycles = sim_cycle - pending_invalid_start_cycles[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2];
				}
				free_event(event);
				return 1;	
			}
#endif
			}
			event->src1 += event->des1;
			event->des1 = event->src1 - event->des1;
			event->src1 = event->src1 - event->des1;
			event->src2 += event->des2;
			event->des2 = event->src2 - event->des2;
			event->src2 = event->src2 - event->des2;
			event->processingDone = 0;
			event->startCycle = sim_cycle;
			/* schedule a network access */
			scheduleThroughNetwork(event, sim_cycle, packet_size, vc);

			dir_eventq_insert(event);
			return 1;

		case ACK_MSG_READ	:	
		case ACK_MSG_READUPDATE	:	
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			int Readpending = 0;
#ifdef SERIALIZATION_ACK
			if(event->blk_dir->pendingInvAck[block_offset])
			{
#ifdef READ_PERMIT
				if(event->blk_dir->ReadPending[block_offset] == -1)
				{
					event->parent->blk_dir->ReadPending[block_offset] = event->tempID;
					event->parent->blk_dir->ReadPending_event[block_offset] = event->parent;
					Readpending = 1;
				}
#else
				event->when += RETRY_TIME;
				dir_eventq_insert(event);
				return 1;
#endif
			}
#endif
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(event->blk_dir->dir_state[block_offset] == DIR_STABLE)
				panic("DIR: after the receipt of ACK msg, directory not in transient state");
			(event->parent)->childCount --;
			if(collect_stats)
			{
				//cache_dl2->hits ++;
				if(event->operation == ACK_MSG_READUPDATE)
					cache_dl2->dir_access ++;
				else
					cache_dl2->data_access ++;
			}
			if((event->parent)->childCount == 0)
			{
				struct DIRECTORY_EVENT *parent_event = event->parent;
				tempID = parent_event->tempID;
				if((parent_event->blk_dir)->dir_blk_state[block_offset] == MESI_SHARED)
					panic("DIR: L2 cache line could not be in shared state");
				/* After receiving this message, the directory information should be changed*/
				(parent_event->blk_dir)->ptr_cur_event[block_offset] = NULL;
				(parent_event->blk_dir)->dir_dirty[block_offset] = -1;
				(parent_event->blk_dir)->dir_sharer[block_offset][tempID/64] |= (unsigned long long int)1<<(tempID%64);
				(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_SHARED;
				if(event->operation == ACK_MSG_READ)
					(parent_event->blk_dir)->status |= CACHE_BLK_DIRTY;

				if(event->l1LineUseStatus)
				{
					delayL2ReadReqDirty += event->when - event->parent->when;
					totalL2ReadReqDirtyWT++;
				}

#ifdef READ_PERMIT   /* jing090107 */
				if(!Readpending)
				{
#endif
					(parent_event->blk_dir)->dir_state[block_offset] = DIR_STABLE;
					/* My stats */
					timespace(sim_cycle - parent_event->blk_dir->exclusive_time[block_offset], 1);  /* 1 means read */

					/* send acknowledgement from directory to requester to changed state into shared */
#ifdef PREFETCH_OPTI
					if(parent_event->operation == READ_COHERENCE_UPDATE)
					{
						if(event->operation == ACK_MSG_READUPDATE)
						{
							if(parent_event->blk_dir->prefetch_modified[block_offset] || parent_event->blk_dir->prefetch_invalidate[block_offset])
							{
								packet_size = data_packet_size;
								parent_event->operation = READ_META_REPLY_SHARED;
								ReadEDataReply ++;
							}
							else
							{
								packet_size = meta_packet_size;
								parent_event->operation = READ_META_REPLY_SHARED;
								ReadEMetaReply ++;
							}
						}
						else
						{
							packet_size = data_packet_size;
							parent_event->operation = READ_DATA_REPLY_SHARED;
							ReadMReply ++;
						}
						if(parent_event->blk_dir->dir_prefetch_num[block_offset] == 0)
						{
							parent_event->blk_dir->prefetch_invalidate[block_offset] = 0;
							parent_event->blk_dir->prefetch_modified[block_offset] = 0;
						}
					}
					else
					{
						packet_size = data_packet_size;
						parent_event->operation = ACK_DIR_READ_SHARED;
					}
#else
					parent_event->operation = ACK_DIR_READ_SHARED;
					packet_size = data_packet_size;
#endif
					parent_event->reqAckTime = sim_cycle;
					parent_event->src1 = event->des1;
					parent_event->src2 = event->des2;
					parent_event->des1 = tempID/mesh_size+MEM_LOC_SHIFT;
					parent_event->des2 = tempID%mesh_size;
					parent_event->processingDone = 0;
					parent_event->startCycle = sim_cycle;
					parent_event->pendingInvAck = 0;
					scheduleThroughNetwork(parent_event, sim_cycle, packet_size, vc);
#ifdef LOCK_CACHE_REGISTER
					/* update the lock register if necessary */
					if(parent_event->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
						LockRegesterCheck(parent_event);
#endif				
					free_event(event);
					dir_eventq_insert(parent_event);
#ifdef READ_PERMIT   /* jing090107 */
				}
				else
				{
					free_event(event);
					//free_event(event->parent);
				}
#endif
				return 1;
			}
			break;
#ifdef LOCK_CACHE_REGISTER
		case ACK_MSG_UPDATE:
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(event->parent->blk_dir->dir_state[block_offset] == DIR_STABLE)
				panic("DIR: after the receipt of ACK msg, directory not in transient state");
			(event->parent)->childCount --;

			if(collect_stats)
			{
				cache_dl2->ack_received ++;
				cache_dl2->dir_access ++;
			}
			if((event->parent)->childCount == 0)
			{
				/* Receiving all the invalidation acknowledgement Messages */
				counter_t startCycle = sim_cycle;
				struct DIRECTORY_EVENT *parent_event = event->parent;
				tempID = parent_event->tempID;
				/* After receiving this message, the directory information should be changed*/
				(parent_event->blk_dir)->dir_state[block_offset] = DIR_STABLE;
				(parent_event->blk_dir)->ptr_cur_event[block_offset] = NULL;

				/* send acknowledgement from directory to requester to write anyway */		    
				packet_size = meta_packet_size;
				parent_event->operation = ACK_DIR_READ_SHARED;
				parent_event->reqAckTime = sim_cycle;
				parent_event->startCycle = startCycle;
				parent_event->src1 = event->des1;
				parent_event->src2 = event->des2;
				parent_event->des1 = parent_event->tempID/mesh_size+MEM_LOC_SHIFT;
				parent_event->des2 = parent_event->tempID%mesh_size;
				parent_event->processingDone = 0;
				parent_event->pendingInvAck = 0;
				scheduleThroughNetwork(parent_event, parent_event->startCycle, packet_size, vc);
#ifdef LOCK_CACHE_REGISTER
				/* update the lock register if necessary */
				if(parent_event->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
					LockRegesterCheck(parent_event);
#endif				
				free_event(event);

				dir_eventq_insert(parent_event);
				return 1;
			}
			free_event(event);
		break;
#endif
		case ACK_MSG_WRITEUPDATE:	
		case ACK_MSG_WRITE	:	
#ifdef SERIALIZATION_ACK
			invCollectStatus = invAckUpdate((event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2, event->addr, event->tempID, event->isSyncAccess);
			if(invCollectStatus == 1)	
			{
				event->blk_dir->pendingInvAck[block_offset] = 0;
#ifdef READ_PERMIT
				if(event->blk_dir->ReadPending[block_offset] != -1)
				{/* Send final read ack to all the sharers */
					event->blk_dir->dir_state[block_offset] = DIR_STABLE;
					struct DIRECTORY_EVENT *p_event;
					p_event = event->blk_dir->ReadPending_event[block_offset];
					if(p_event->tempID != event->blk_dir->ReadPending[block_offset])
						panic("Event is not the same in Early Read permit!\n");
					p_event->operation = ACK_DIR_READ_SHARED;
					p_event->src1 = event->des1;
					p_event->src2 = event->des2;
					p_event->des1 = p_event->tempID/mesh_size+MEM_LOC_SHIFT;
					p_event->des2 = p_event->tempID%mesh_size;
					p_event->processingDone = 0;
					p_event->startCycle = sim_cycle;
					p_event->pendingInvAck = 0;
					scheduleThroughNetwork(p_event, sim_cycle, data_packet_size, vc);
					dir_eventq_insert(p_event);
					event->blk_dir->ReadPending[block_offset] = -1;
				}
				if(!mystricmp (network_type, "FSOI") || !mystricmp(network_type, "HYBRID"))
				{
#ifdef EUP_NETWORK
				if(event->parent->early_flag == 1)
				{
					if(event->addr == event->parent->addr)
						free_event(event->parent);
				}
#endif
				}
#endif				
			}
			if(invCollectStatus != 0)
			{/* we can complete the event here */	
				free_event(event);
				return 1;
			}
#endif
			dcache2_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache2_access++;
			if(md_text_addr(addr, tempID))
				panic("Writes cannot access text memory region");
			if(event->parent->blk_dir->dir_state[block_offset] == DIR_STABLE)
				panic("DIR: after the receipt of ACK msg, directory not in transient state");
			(event->parent)->blk_dir->dir_sharer[block_offset][(((event->src1-MEM_LOC_SHIFT)*mesh_size+event->src2)/64)] &= ~ ((unsigned long long int)1 << (((event->src1-MEM_LOC_SHIFT)*mesh_size+event->src2)%64));
			(event->parent)->childCount --;

			if((event->parent)->operation == WAIT_MEM_READ)
				(event->parent)->individual_childCount[block_offset] --;
			if(collect_stats)
			{
				if(event->operation == ACK_MSG_WRITEUPDATE)
				{
					cache_dl2->ack_received ++;
					cache_dl2->dir_access ++;
				}
				else
					cache_dl2->data_access ++;
				//cache_dl2->hits ++;
			}
			if((event->parent)->operation == WAIT_MEM_READ && (event->parent)->individual_childCount[block_offset] == 0)	
			{  
				(event->parent)->sharer_num --;
				((event->parent)->blk_dir)->dir_dirty[block_offset] = -1;
				((event->parent)->blk_dir)->dir_sharer[block_offset][0] = 0;
				((event->parent)->blk_dir)->dir_sharer[block_offset][1] = 0;
				((event->parent)->blk_dir)->dir_sharer[block_offset][2] = 0;
				((event->parent)->blk_dir)->dir_sharer[block_offset][3] = 0;

				if((event->parent)->sharer_num != 0)
				{
					free_event(event);
					return 1;
				}
			}

			if((event->parent)->childCount == 0)
			{
				/* Receiving all the invalidation acknowledgement Messages */
				counter_t startCycle = sim_cycle;
				struct DIRECTORY_EVENT *parent_event = event->parent;
				tempID = parent_event->tempID;
				/* After receiving this message, the directory information should be changed*/
				(parent_event->blk_dir)->dir_state[block_offset] = DIR_STABLE;
				(parent_event->blk_dir)->ptr_cur_event[block_offset] = NULL;
				block_offset = blockoffset(parent_event->addr);
				(parent_event->blk_dir)->dir_dirty[block_offset] = tempID;
				(parent_event->blk_dir)->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
				(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_MODIFIED;

				if(event->operation == ACK_MSG_WRITE)
					(parent_event->blk_dir)->status |= CACHE_BLK_DIRTY;
				if(parent_event->operation != WAIT_MEM_READ)
				{
					(parent_event->blk_dir)->dir_dirty[block_offset] = tempID;
					(parent_event->blk_dir)->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
					(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_MODIFIED;
					/* My stats */
					if(parent_event->originalChildCount == 1)
						timespace(sim_cycle - parent_event->blk_dir->exclusive_time[block_offset], 2);  /* 2 means read */
#ifdef PREFETCH_OPTI
					if(parent_event->operation != WRITE_COHERENCE_UPDATE)
					{
						if((parent_event->blk_dir)->dir_prefetch_num[block_offset])
							(parent_event->blk_dir)->prefetch_invalidate[block_offset] = 1;	
						if((parent_event->blk_dir)->dir_prefetch_num[block_offset])
							(parent_event->blk_dir)->prefetch_modified[block_offset] = 1;	
					}
#endif

					if(event->l1LineUseStatus)
					{
						if(event->isExclusiveOrDirty)
						{
							delayL2WriteReqDirty += event->when - event->parent->when;
							totalL2WriteReqDirtyWT++;
						}
						else 
						{
							delayL2WriteReqShared += event->when - event->parent->when;
							totalL2WriteReqSharedWT++;
						}
					}

					if(event->operation == ACK_MSG_WRITE)
						(parent_event->blk_dir)->status |= CACHE_BLK_DIRTY;


					/* send acknowledgement from directory to requester to changed state into modified */		    
					if(parent_event->operation == WRITE_UPDATE)
					{ /* WRITE HIT */
						packet_size = meta_packet_size;
						parent_event->operation = ACK_DIR_WRITEUPDATE;
					}
					else if(parent_event->operation == MISS_WRITE)
					{ /* WRITE MISS */	
						packet_size = data_packet_size;
						parent_event->operation = ACK_DIR_WRITE;
					} 
#ifdef PREFETCH_OPTI
					else if(parent_event->operation == WRITE_COHERENCE_UPDATE)
					{
						if(event->operation == ACK_MSG_WRITE)
						{
							packet_size = data_packet_size;
							parent_event->operation = WRITE_DATA_REPLY;
							WriteMReply ++;
						}
						else
						{
							if(parent_event->blk_dir->prefetch_invalidate[block_offset] || parent_event->blk_dir->prefetch_modified[block_offset])
							{
								packet_size = data_packet_size;
								parent_event->operation = WRITE_DATA_REPLY;
								WriteSEDataReply ++;
							}
							else
							{
								packet_size = meta_packet_size;
								parent_event->operation = WRITE_META_REPLY;
								WriteSEMetaReply ++;
							}	
						}
						if(parent_event->blk_dir->dir_prefetch_num[block_offset] == 0)
						{
							parent_event->blk_dir->prefetch_invalidate[block_offset] = 0;
							parent_event->blk_dir->prefetch_modified[block_offset] = 0;
						}
					} 
#endif
				
				}
				else
				{  /* finish the replacement, then have to finish the memory read*/
					if(event->sharer_num != 0)
						panic("WAIT_MEM_READ: sharer_num should be zero now!\n");


					/* L2 write back latency */
					if(!parent_event->blk_dir->Iswb_to_mem && (parent_event->blk_dir->status & CACHE_BLK_DIRTY) && (parent_event->blk_dir->status & CACHE_BLK_VALID))
					{
						int new_vc = 0;
						int l=0;
						for(l=0;l<cache_dl2->bsize/max_packet_size;l++)
						{
							struct DIRECTORY_EVENT *new_event;
							new_event = allocate_new_event(parent_event);
							new_event->addr = parent_event->addr;
							new_event->operation = MEM_WRITE_BACK;
							new_event->cmd = Write;
							scheduleThroughNetwork(new_event, sim_cycle, data_packet_size, new_vc);
							dir_eventq_insert(new_event);
						}	
						parent_event->blk_dir->Iswb_to_mem = 1;
						if(collect_stats)
							cache_dl2->writebacks ++;
					}
					else	
						parent_event->blk_dir->Iswb_to_mem = 0;

					tag_dir = CACHE_TAG(cp_dir, parent_event->addr);	//Tag address is parent address 

					int i=0, m=0, iteration = 1;
					for(i=0; i<(cache_dl2->set_shift - cache_dl1[0]->set_shift); i++)
						iteration = iteration * 2; 
					for(m=0; m<iteration; m++)
						(parent_event->blk_dir)->dir_state[m] = DIR_STABLE;

					(parent_event->blk_dir)->tagid.tag = tag_dir;
					(parent_event->blk_dir)->tagid.threadid = tempID;	
					(parent_event->blk_dir)->addr = parent_event->addr;
					(parent_event->blk_dir)->status = CACHE_BLK_VALID;
					(parent_event->blk_dir)->Iswb_to_mem = 0;

					/* jing: Here is a problem: copy data out of cache block, if block exists */
					/* do something here */

					cp_dir->last_tagsetid.tag = 0;
					cp_dir->last_tagsetid.threadid = tempID;
					cp_dir->last_blk = (parent_event->blk_dir);

					/* get user block data, if requested and it exists */
					if (parent_event->udata)
						*(parent_event->udata) = (parent_event->blk_dir)->user_data;

					(parent_event->blk_dir)->ready = sim_cycle;

					if (cp_dir->hsize)
						link_htab_ent(cp_dir, &cp_dir->sets[set_dir], (parent_event->blk_dir));

					/* remove the miss entry in L2 MSHR*/
					update_L2_mshrentry(parent_event->addr, sim_cycle, parent_event->startCycle);
					(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_MODIFIED;
#ifdef PREFETCH_OPTI
					for(i=0;i<MAXTHREADS;i++)
						(parent_event->blk_dir)->dir_prefetch[block_offset][i] = 0;
					(parent_event->blk_dir)->dir_prefetch_num[block_offset] = 0;
					(parent_event->blk_dir)->prefetch_invalidate[block_offset] = 0;
					(parent_event->blk_dir)->prefetch_modified[block_offset] = 0;
#endif

					if(parent_event->parent_operation == L2_PREFETCH)
					{	
						(parent_event->blk_dir)->dir_sharer[block_offset][0] = 0;
						(parent_event->blk_dir)->dir_sharer[block_offset][1] = 0;
						(parent_event->blk_dir)->dir_sharer[block_offset][2] = 0;
						(parent_event->blk_dir)->dir_sharer[block_offset][3] = 0;
						(parent_event->blk_dir)->dir_dirty[block_offset] = -1;
						free_event(event);
						free_event(parent_event);
						return 1;
					}

					if(parent_event->parent_operation == MISS_IL1)
					{
						packet_size = data_packet_size;
						parent_event->operation = ACK_DIR_IL1;
						(parent_event->blk_dir)->dir_dirty[block_offset] = -1;
						(parent_event->blk_dir)->dir_sharer[block_offset][0] = 0;
						(parent_event->blk_dir)->dir_sharer[block_offset][1] = 0;
						(parent_event->blk_dir)->dir_sharer[block_offset][2] = 0;
						(parent_event->blk_dir)->dir_sharer[block_offset][3] = 0;
						(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_SHARED;
						startCycle = sim_cycle;
					}
					else if(parent_event->parent_operation == MISS_READ 
#ifdef PREFETCH_OPTI
					|| parent_event->parent_operation == L1_PREFETCH
					|| parent_event->parent_operation == READ_COHERENCE_UPDATE
#endif
					)
					{
						packet_size = data_packet_size;
						parent_event->operation = ACK_DIR_READ_EXCLUSIVE;
						(parent_event->blk_dir)->dir_dirty[block_offset] = -1;
						if (!md_text_addr(parent_event->addr, tempID))
						{
							(parent_event->blk_dir)->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
							(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
						}
						else
						{
							(parent_event->blk_dir)->dir_sharer[block_offset][0] = 0;
							(parent_event->blk_dir)->dir_sharer[block_offset][1] = 0;
							(parent_event->blk_dir)->dir_sharer[block_offset][2] = 0;
							(parent_event->blk_dir)->dir_sharer[block_offset][3] = 0;
							(parent_event->blk_dir)->dir_blk_state[block_offset] = MESI_SHARED;
						}
#ifdef LOCK_CACHE_REGISTER
						/* update the lock register if necessary */
						if(parent_event->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
							LockRegesterCheck(parent_event);
#endif				
					}
					else if(parent_event->parent_operation == MISS_WRITE
#ifdef PREFETCH_OPTI
					|| parent_event->parent_operation == WRITE_COHERENCE_UPDATE
#endif
					)
					{
						packet_size = data_packet_size;
						parent_event->operation = ACK_DIR_WRITE;
						(parent_event->blk_dir)->dir_dirty[block_offset] = tempID;
						(parent_event->blk_dir)->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
					}
					else
					{
						packet_size = meta_packet_size;
						parent_event->operation = ACK_DIR_WRITEUPDATE;
						(parent_event->blk_dir)->dir_dirty[block_offset] = tempID;
						(parent_event->blk_dir)->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
					}
				}
				/* My stats */
				parent_event->blk_dir->exclusive_time[block_offset] = sim_cycle;
				parent_event->reqAckTime = sim_cycle;
				parent_event->startCycle = startCycle;
				parent_event->src1 = event->des1;
				parent_event->src2 = event->des2;
				parent_event->des1 = parent_event->tempID/mesh_size+MEM_LOC_SHIFT;
				parent_event->des2 = parent_event->tempID%mesh_size;
				parent_event->processingDone = 0;
				parent_event->pendingInvAck = 0;
				scheduleThroughNetwork(parent_event, parent_event->startCycle, packet_size, vc);
				free_event(event);

				dir_eventq_insert(parent_event);
				return 1;
			}
			free_event(event);
			break;

		case ACK_DIR_IL1	:
			icache_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->icache_access++;
			/* First find the replacement line */
			tag = CACHE_TAG(event->cp, addr);
			set = CACHE_SET(event->cp, addr);

			switch ((event->cp)->policy)
			{
				int bindex;
				case LRU:
				case FIFO:
				repl = (event->cp)->sets[set].way_tail;
				update_way_list (&(event->cp)->sets[set], repl, Head);
				break;
				case Random:
				bindex = myrand () & ((event->cp)->assoc - 1);
				repl = CACHE_BINDEX ((event->cp), (event->cp)->sets[set].blks, bindex);
				break;
				default:
				panic ("bogus replacement policy");
			}
			if (collect_stats)
				event->cp->replacements++;
			repl->tagid.tag = tag;
			repl->tagid.threadid = tempID;
			repl->addr = event->addr;
			repl->status = CACHE_BLK_VALID;           
			repl->state = MESI_SHARED;

			/* copy data out of cache block, if block exists */
			bofs = CACHE_BLK(event->cp, event->addr);
			if ((event->cp)->balloc && (event->vp))
				CACHE_BCOPY(event->cmd, repl, bofs, event->vp, event->nbytes);

			(event->cp)->last_tagsetid.tag = 0;
			(event->cp)->last_tagsetid.threadid = tempID;
			(event->cp)->last_blk = repl;

			/* get user block data, if requested and it exists */
			if (event->udata)
				*(event->udata) = repl->user_data;

			if(collect_stats)
				totalIL1misshandletime += sim_cycle - event->started;

			repl->ready = sim_cycle;

			if ((event->cp)->hsize)
				link_htab_ent((event->cp), &(event->cp)->sets[set], repl);

			thecontexts[tempID]->ruu_fetch_issue_delay = 0;
			thecontexts[tempID]->event = NULL;
			free_event(event);

			break;

#ifdef SERIALIZATION_ACK
		case FINAL_INV_ACK:
			if(numStorePending[event->tempID] == 0)
				panic("error in FINAL_INV_ACK\n");
			numStorePending[event->tempID]--;
			if((numStorePending[event->tempID] == 0) && thecontexts[event->tempID]->WBtableTail == NULL)
				thecontexts[event->tempID]->atMB = 0;
			free_event(event);
			break;

#endif

#ifdef WRITE_EARLY
		case ACK_DIR_WRITE	:
			if(event->early_flag == 3)
			{
				cp1 = event->cp;
				write_early --;
				tag = CACHE_TAG(event->cp,addr);
				set = CACHE_SET(event->cp,addr);
				int flag = 0;
				for (blk1 = cp1->sets[set].way_head; blk1; blk1 =blk1->way_next)
				{
					if ((blk1->tagid.tag == tag) && (blk1->status & CACHE_BLK_WVALID))
					{
						flag = 1;
						blk1->status &= (~CACHE_BLK_WVALID);
						blk1->status |= CACHE_BLK_VALID;
						break;
					}
				}
				if(flag != 0)
				{
#ifdef SERIALIZATION_ACK	
					if(event->pendingInvAck)
						numStorePending[tempID]++;
#endif
					mshrentry_update(event, sim_cycle, event->started);
					free_event(event);
					return 1;	
				}
			}
		case ACK_DIR_WRITE_EARLY:
			dcache_access++;
			//if(event->operation == ACK_DIR_WRITE_EARLY)
			write_early ++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache_access++;
			/* First find the replacement line */
			tag = CACHE_TAG(event->cp, addr);
			set = CACHE_SET(event->cp, addr);

			switch ((event->cp)->policy)
			{
				int bindex, i=0;
				case LRU:
				case FIFO:
				/*
				repl = (event->cp)->sets[set].way_tail;
				if(event->operation != ACK_DIR_WRITE_EARLY)
					update_way_list (&(event->cp)->sets[set], repl, Head);
				break;
				*/
				i = event->cp->assoc;
				do {
					if(i == 0)	break;
					i--;
					repl = event->cp->sets[set].way_tail;
					update_way_list (&(event->cp)->sets[set], repl, Head);
				}
				while(repl->status & CACHE_BLK_WVALID);

				if(repl->status & CACHE_BLK_WVALID)
				{
					L1_flip_counter ++;
					event->when = sim_cycle + RETRY_TIME;
					dir_eventq_insert(event);
					return 1;
				}
				break;
				case Random:
				bindex = myrand () & ((event->cp)->assoc - 1);
				repl = CACHE_BINDEX ((event->cp), (event->cp)->sets[set].blks, bindex);
				break;
				default:
				panic ("bogus replacement policy");
			}

			if ((repl->status & CACHE_BLK_VALID) 
#ifdef SILENT_DROP
#ifndef UPDATE_HINT
					&& (repl->status & CACHE_BLK_DIRTY)
#endif
#endif
			   )
			{
				if (collect_stats)
					event->cp->replacements++;

				if (actualProcess == 1 && !md_text_addr(repl->addr, tempID))     /*no write back to L2 for dual exec */
				{
					if ((repl->status & CACHE_BLK_DIRTY) && !(repl->state == MESI_MODIFIED))
						panic ("Cache Block should have been modified here");

					struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
					if(new_event == NULL) 	panic("Out of Virtual Memory");
					new_event->isPrefetch = 0;
#if defined(NON_SILENT_DROP) || defined(UPDATE_HINT)
					if (repl->status & CACHE_BLK_DIRTY)
					{
#endif
						if (collect_stats)
							event->cp->writebacks++;
#ifdef WB_SPLIT
						new_event->operation = WRITE_BACK_HEAD;
						packet_size = meta_packet_size;
						int j=0, wb_f;
						for(j=0;j<MSHR_SIZE;j++)
						{
							if(wb_buffer[event->des1*mesh_size+event->des2][j] == 0)
							{
								wb_buffer[event->des1*mesh_size+event->des2][j] = repl->addr;
								wb_f = 1;	
								break;
							}		
						}
#else
						new_event->operation = WRITE_BACK;
						packet_size = data_packet_size;
#endif
#if defined(NON_SILENT_DROP) || defined(UPDATE_HINT)
					}
					else
					{
						new_event->operation = UPDATE_DIR;
						packet_size = meta_packet_size;
						if (collect_stats)
							event->cp->dir_notification++;
					} 
#endif
					new_event->conf_bit = -1;
					new_event->req_time = 0;
					new_event->input_buffer_full = 0;
					new_event->pending_flag = 0;
					new_event->op = event->op;
					new_event->src1 = event->des1;
					new_event->src2 = event->des2;
					md_addr_t src = (repl->addr >> cache_dl2->set_shift) % numcontexts;
					new_event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
					new_event->des2 = (src %mesh_size);
					new_event->processingDone = 0;
					new_event->startCycle = sim_cycle;
					new_event->parent = NULL;
					new_event->blk_dir = NULL;
					new_event->childCount = 0;
					new_event->tempID = event->tempID;
					new_event->L2miss_flag = event->L2miss_flag;
					new_event->resend = 0;
					new_event->blk1 = repl;
					new_event->addr = repl->addr;
					new_event->cp = event->cp;
					new_event->vp = NULL;
					new_event->nbytes = event->cp->bsize;
					new_event->udata = NULL;
					new_event->cmd = Write;
					new_event->rs = NULL; 
					new_event->spec_mode = 0;
					new_event->data_reply = 0;
					int new_vc = 0;
					/*go to network*/
					scheduleThroughNetwork(new_event, sim_cycle, packet_size, new_vc);

					dir_eventq_insert(new_event);
				}
			}
			
			if(repl->status & CACHE_BLK_VALID)
			{
				if(repl->state == MESI_EXCLUSIVE)
					totalL1LineExlDrop++;

				if(repl->blkAlocReason == LINE_PREFETCH)
				{
					if(repl->blkImdtOp)
						totalL1LinePrefUse++;
					else
						totalL1LinePrefNoUse++;
				}
				else if(repl->blkAlocReason == LINE_READ)
				{
					if(repl->blkImdtOp)
						totalL1LineReadUse++;
					else
						totalL1LineReadNoUse++;
				}
				else if(repl->blkAlocReason == LINE_WRITE)
				{
					if(repl->blkImdtOp)
						totalL1LineWriteUse++;
					else
						totalL1LineWriteNoUse++;
				}
			}

			if(event->isPrefetch)
				repl->blkAlocReason = LINE_PREFETCH;
			else if(event->cmd == Write)
				repl->blkAlocReason = LINE_WRITE;
			else if(event->cmd == Read)
				repl->blkAlocReason = LINE_READ;
			else
				repl->blkAlocReason = 0;

			/* update block tags */
			repl->tagid.tag = tag;
			repl->tagid.threadid = tempID;
			repl->addr = event->addr;
			if(event->operation == ACK_DIR_WRITE_EARLY)
				repl->status = CACHE_BLK_WVALID;           
			else 
				repl->status = CACHE_BLK_VALID;

			repl->blkImdtOp = 0;
			repl->blkNextOps = 0;

#ifdef SERIALIZATION_ACK	
			if(event->operation != ACK_DIR_WRITE_EARLY && event->pendingInvAck)
				numStorePending[tempID]++;
#endif

			/* copy data out of cache block, if block exists */
			bofs = CACHE_BLK(event->cp, event->addr);
			if ((event->cp)->balloc && (event->vp))
				CACHE_BCOPY(event->cmd, repl, bofs, event->vp, event->nbytes);

			/* update cache block status at requester L1 */
			//repl->status = CACHE_BLK_VALID;
			if(event->operation == ACK_DIR_READ_EXCLUSIVE) 
				repl->state = MESI_EXCLUSIVE;
			else if(event->operation == ACK_DIR_READ_SHARED)
				repl->state = MESI_SHARED;
			else if(event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE || event->operation == ACK_DIR_WRITE_EARLY) 
			{
				repl->state = MESI_MODIFIED;
				repl->status |= CACHE_BLK_DIRTY;
			}

			(event->cp)->last_tagsetid.tag = 0;
			(event->cp)->last_tagsetid.threadid = tempID;
			(event->cp)->last_blk = repl;

			/* get user block data, if requested and it exists */
			if (event->udata)
				*(event->udata) = repl->user_data;

			if(collect_stats)
			{
				//if(!event->isPrefetch)
				{
					int missHandleTime = sim_cycle - event->started;
					if(!event->isReqL2Hit)
					{
						totalL2ReqPrimMiss++;
						delayL2ReqPrimMiss += missHandleTime;
					}
					else
					{
						if(event->isReqL2SecMiss)
						{
							totalL2ReqSecMiss++;
							delayL2ReqSecMiss += missHandleTime;
						}
						else if(event->isReqL2Trans)
						{
							totalL2ReqInTrans++;
							delayL2ReqInTrans += missHandleTime;
							if(event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE)
							{
								totalL2OwnReqInTrans++;
								delayL2OwnReqInTrans += missHandleTime;
							}
						}
						else if(event->isReqL2Inv)
						{
							totalL2ReqInInv++;
							delayL2ReqInInv += missHandleTime;
						}
						else
						{
							totalL2ReqHit++;
							delayL2ReqHit += missHandleTime;
						}
					}
				}

				if(event->operation == ACK_DIR_WRITE && event->isReqL2Hit && !event->isReqL2SecMiss)
				{
					totalWriteReq++;
					totalWriteReqInv += ((event->reqAckTime == event->reqAtDirTime) ? 0 : 1);
					totalWriteDelay += (sim_cycle - event->started);
					totalWriteInvDelay += (event->reqAckTime - event->reqAtDirTime);
				}
				if(event->operation == ACK_DIR_WRITEUPDATE && event->isReqL2Hit && !event->isReqL2SecMiss)
				{
					totalUpgradeReq++;
					totalUpgradeReqInv += ((event->reqAckTime == event->reqAtDirTime) ? 0 : 1);
					totalUpgradeDelay += (sim_cycle - event->started);
					totalUpgradeInvDelay += (event->reqAckTime - event->reqAtDirTime);
				}

				totalmisshandletime += sim_cycle - event->started;
				if(event->isSyncAccess)
					totalmisstimeforSync += sim_cycle - event->started;
				else 
					totalmisstimeforNormal += sim_cycle - event->started;
				
					
				//printf("Miss completed %d %llx: %lld, %lld (%lld), %lld (%d)(%lld), %lld (%lld), %lld (%lld)\n", event->isSyncAccess, event->addr, event->started, event->reqNetTime, event->reqNetTime-event->started, event->reqAtDirTime, event->isReqL2Hit, event->reqAtDirTime-event->reqNetTime, event->reqAckTime,  event->reqAckTime-event->reqAtDirTime, sim_cycle, sim_cycle-event->reqAckTime); 
			}
			repl->ready = sim_cycle;

			if ((event->cp)->hsize)
				link_htab_ent((event->cp), &(event->cp)->sets[set], repl);

			/* Update the rs finish time in RS_LINK (event_queue) and mshr entry */

			if(!event->spec_mode && event->rs)
			{
				if((event->operation == ACK_DIR_READ_SHARED) || (event->operation == ACK_DIR_READ_EXCLUSIVE))
					eventq_update(event->rs, repl->ready);
				else
					event->rs->writewait = 2;
			}
			if(event->operation != ACK_DIR_WRITE_EARLY)
				mshrentry_update(event, repl->ready, event->started);

			free_event(event);
			break;
#else
		case ACK_DIR_WRITE	:
#endif
		case ACK_DIR_WRITEUPDATE:
		case ACK_DIR_READ_EXCLUSIVE:
		case ACK_DIR_READ_SHARED:
			dcache_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache_access++;
			if(event->operation == ACK_DIR_READ_SHARED)
				data_shared_read ++;
			else if(event->operation == ACK_DIR_READ_EXCLUSIVE)
				data_private_read ++;	
#ifdef LOCK_CACHE_REGISTER
			if(event->operation == ACK_DIR_READ_SHARED)
			{
				if(event->op == LL)
				{
					if (common_regs_s[thecontexts[tempID]->masterid][tempID].address == event->addr && common_regs_s[thecontexts[tempID]->masterid][tempID].regs_lock)
					{
						common_regs_s[thecontexts[tempID]->masterid][tempID].subscribed = 1;
					}
				}
				else if(event->op == SC)
				{ /* for store_conditional, clear the subscribed bit after it sucessful */
					if (common_regs_s[thecontexts[tempID]->masterid][tempID].address == event->addr && common_regs_s[thecontexts[tempID]->masterid][tempID].regs_lock)
					{
						common_regs_s[thecontexts[tempID]->masterid][tempID].regs_lock = 0;
						common_regs_s[thecontexts[tempID]->masterid][tempID].address = 0;
						common_regs_s[thecontexts[tempID]->masterid][tempID].subscribed = 0;
					}
				}
			}
#endif
			/* First find the replacement line */
			tag = CACHE_TAG(event->cp, addr);
			set = CACHE_SET(event->cp, addr);
			if(event->operation == ACK_DIR_WRITEUPDATE)
				m_update_miss[event->tempID] = 0;	

			switch ((event->cp)->policy)
			{
				int bindex, i=0;
				case LRU:
				case FIFO:
				if(!mystricmp (network_type, "FSOI") || !mystricmp(network_type, "HYBRID"))
				{
#ifdef WRITE_EARLY
				i = event->cp->assoc;
				do {
					if(i == 0)	break;
					i--;
					repl = (event->cp)->sets[set].way_tail;
					update_way_list (&(event->cp)->sets[set], repl, Head);
				}
				while(repl->status & CACHE_BLK_WVALID);

				if(repl->status & CACHE_BLK_WVALID)
				{
					L1_flip_counter ++;
					event->when = sim_cycle + RETRY_TIME;
					dir_eventq_insert(event);
					return 1;
				}
#else
				repl = (event->cp)->sets[set].way_tail;
				update_way_list (&(event->cp)->sets[set], repl, Head);
#endif
				}
				else
				{
					repl = (event->cp)->sets[set].way_tail;
					update_way_list (&(event->cp)->sets[set], repl, Head);
				}
				break;
				case Random:
				bindex = myrand () & ((event->cp)->assoc - 1);
				repl = CACHE_BINDEX ((event->cp), (event->cp)->sets[set].blks, bindex);
				break;
				default:
				panic ("bogus replacement policy");
			}

			if ((repl->status & CACHE_BLK_VALID) 
#ifdef SILENT_DROP
#ifndef UPDATE_HINT
					&& (repl->status & CACHE_BLK_DIRTY)
#endif
#endif
			   )
			{
				if (collect_stats)
					event->cp->replacements++;

				if (actualProcess == 1 && !md_text_addr(repl->addr, tempID))     /*no write back to L2 for dual exec */
				{
					if ((repl->status & CACHE_BLK_DIRTY) && !(repl->state == MESI_MODIFIED))
						panic ("Cache Block should have been modified here");

					struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
					if(new_event == NULL) 	panic("Out of Virtual Memory");
					new_event->isPrefetch = 0;
#if defined(NON_SILENT_DROP) || defined(UPDATE_HINT)
					if (repl->status & CACHE_BLK_DIRTY)
					{
#endif
						if (collect_stats)
							event->cp->writebacks++;

						int m = 0;	
						for(m=0;m<8;m++)
							if(repl->WordUseFlag[m])
								repl->WordCount ++;
						stats_do((repl->addr & ~(cache_dl1[0]->bsize-1)), repl->ReadCount, repl->WriteCount, repl->WordCount, repl->Type, tempID);
						repl->ReadCount = 0;	
						repl->WriteCount = 0;	
						repl->WordCount = 0;	
						for(m=0;m<8;m++)
							repl->WordUseFlag[m] = 0;
#ifdef WB_SPLIT
						new_event->operation = WRITE_BACK_HEAD;
						packet_size = meta_packet_size;
						int j=0, wb_f;
						for(j=0;j<MSHR_SIZE;j++)
						{
							if(wb_buffer[event->des1*mesh_size+event->des2][j] == 0)
							{
								wb_buffer[event->des1*mesh_size+event->des2][j] = repl->addr;
								wb_f = 1;	
								break;
							}		
						}
						if(wb_f == 0)
							printf("Wb_addr pending buffer is full\n");
#else
						new_event->operation = WRITE_BACK;
						packet_size = data_packet_size;
#endif
#if defined(NON_SILENT_DROP) || defined(UPDATE_HINT)
					}
					else
					{
						new_event->operation = UPDATE_DIR;
						packet_size = meta_packet_size;
						if (collect_stats)
							event->cp->dir_notification++;
					} 
#endif
					new_event->conf_bit = -1;
					new_event->req_time = 0;
					new_event->input_buffer_full = 0;
					new_event->pending_flag = 0;
					new_event->op = event->op;
					new_event->src1 = event->des1;
					new_event->src2 = event->des2;
					md_addr_t src = (repl->addr >> cache_dl2->set_shift) % numcontexts;
					new_event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
					new_event->des2 = (src %mesh_size);
					new_event->processingDone = 0;
					new_event->startCycle = sim_cycle;
					new_event->parent = NULL;
					new_event->blk_dir = NULL;
					new_event->childCount = 0;
					new_event->tempID = event->tempID;
					new_event->L2miss_flag = event->L2miss_flag;
					new_event->resend = 0;
					new_event->blk1 = repl;
					new_event->addr = repl->addr;
					new_event->cp = event->cp;
					new_event->vp = NULL;
					new_event->nbytes = event->cp->bsize;
					new_event->udata = NULL;
					new_event->cmd = Write;
					new_event->rs = NULL; 
					new_event->spec_mode = 0;
					new_event->data_reply = 0;
					int new_vc = 0;
					/*go to network*/
					scheduleThroughNetwork(new_event, sim_cycle, packet_size, new_vc);

					dir_eventq_insert(new_event);
				}
			}
			if(repl->status & CACHE_BLK_VALID)
			{
				if(repl->state == MESI_EXCLUSIVE)
					totalL1LineExlDrop++;

				if(repl->blkAlocReason == LINE_PREFETCH)
				{
					if(repl->blkImdtOp)
						totalL1LinePrefUse++;
					else
						totalL1LinePrefNoUse++;
				}
				else if(repl->blkAlocReason == LINE_READ)
				{
					if(repl->blkImdtOp)
						totalL1LineReadUse++;
					else
						totalL1LineReadNoUse++;
				}
				else if(repl->blkAlocReason == LINE_WRITE)
				{
					if(repl->blkImdtOp)
						totalL1LineWriteUse++;
					else
						totalL1LineWriteNoUse++;
				}
			}

			if(event->isPrefetch)
				repl->blkAlocReason = LINE_PREFETCH;
			else if(event->cmd == Write)
				repl->blkAlocReason = LINE_WRITE;
			else if(event->cmd == Read)
				repl->blkAlocReason = LINE_READ;
			else
				repl->blkAlocReason = 0;

			/* update block tags */
			repl->tagid.tag = tag;
			repl->tagid.threadid = tempID;
			repl->addr = event->addr;
			repl->status = CACHE_BLK_VALID;           

			repl->blkImdtOp = 0;
			repl->blkNextOps = 0;
				

#ifdef SERIALIZATION_ACK	
			if(event->pendingInvAck)
				numStorePending[tempID]++;
#endif

			/* copy data out of cache block, if block exists */
			bofs = CACHE_BLK(event->cp, event->addr);
			if ((event->cp)->balloc && (event->vp))
				CACHE_BCOPY(event->cmd, repl, bofs, event->vp, event->nbytes);

			/* update cache block status at requester L1 */
			repl->WordUseFlag[(repl->addr>>2) & 7] = 1;
			repl->status = CACHE_BLK_VALID;
			if(event->operation == ACK_DIR_READ_EXCLUSIVE) 
			{
				repl->ReadCount ++;
				repl->Type = PRIVATE;
				repl->state = MESI_EXCLUSIVE;
			}
			else if(event->operation == ACK_DIR_READ_SHARED)
			{
				repl->ReadCount ++;
				repl->Type = SHARED_READ_ONLY; //FIXME it can be share read write type data
				repl->state = MESI_SHARED;
			}
			else if(event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE) 
			{
				repl->WriteCount ++;
				if(event->operation == ACK_DIR_WRITEUPDATE)
					repl->Type = SHARED_READ_WRITE;
				else if(event->operation == ACK_DIR_WRITE)
					repl->Type = PRIVATE;  //FIXME: it can be shared read write type data
				repl->state = MESI_MODIFIED;
				repl->status |= CACHE_BLK_DIRTY;
			}

			(event->cp)->last_tagsetid.tag = 0;
			(event->cp)->last_tagsetid.threadid = tempID;
			(event->cp)->last_blk = repl;

			/* get user block data, if requested and it exists */
			if (event->udata)
				*(event->udata) = repl->user_data;

			if(collect_stats)
			{
				//if(!event->isPrefetch)
				{
					int missHandleTime = sim_cycle - event->started;
					if(!event->isReqL2Hit)
					{
						totalL2ReqPrimMiss++;
						delayL2ReqPrimMiss += missHandleTime;
					}
					else
					{
						if(event->isReqL2SecMiss)
						{
							totalL2ReqSecMiss++;
							delayL2ReqSecMiss += missHandleTime;
						}
						else if(event->isReqL2Trans)
						{
							totalL2ReqInTrans++;
							delayL2ReqInTrans += missHandleTime;
							if(event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_WRITEUPDATE)
							{
								totalL2OwnReqInTrans++;
								delayL2OwnReqInTrans += missHandleTime;
							}
						}
						else if(event->isReqL2Inv)
						{
							totalL2ReqInInv++;
							delayL2ReqInInv += missHandleTime;
						}
						else
						{
							totalL2ReqHit++;
							delayL2ReqHit += missHandleTime;
						}
					}
				}

				if(event->operation == ACK_DIR_WRITE && event->isReqL2Hit && !event->isReqL2SecMiss)
				{
					totalWriteReq++;
					totalWriteReqInv += ((event->reqAckTime == event->reqAtDirTime) ? 0 : 1);
					totalWriteDelay += (sim_cycle - event->started);
					totalWriteInvDelay += (event->reqAckTime - event->reqAtDirTime);
				}
				if(event->operation == ACK_DIR_WRITEUPDATE && event->isReqL2Hit && !event->isReqL2SecMiss)
				{
					totalUpgradeReq++;
					totalUpgradeReqInv += ((event->reqAckTime == event->reqAtDirTime) ? 0 : 1);
					totalUpgradeDelay += (sim_cycle - event->started);
					totalUpgradeInvDelay += (event->reqAckTime - event->reqAtDirTime);
				}
				else
					totalUpgradeMiss ++;

				totalmisshandletime += sim_cycle - event->started;
				if(event->isSyncAccess)
					totalmisstimeforSync += sim_cycle - event->started;
				else 
				{
					totalmisstimeforNormal += sim_cycle - event->started;
					if(event->L2miss_flag && event->L2miss_complete && event->L2miss_stated)
					{
						totalL2misstime += event->L2miss_complete - event->L2miss_stated;
						TotalL2misses ++;
					}
					else if(event->L2miss_flag)
						totalWrongL2misstime ++;
				}
				
				//printf("Miss completed %d %llx: %lld, %lld (%lld), %lld (%d)(%lld), %lld (%lld), %lld (%lld)\n", event->isSyncAccess, event->addr, event->started, event->reqNetTime, event->reqNetTime-event->started, event->reqAtDirTime, event->isReqL2Hit, event->reqAtDirTime-event->reqNetTime, event->reqAckTime,  event->reqAckTime-event->reqAtDirTime, sim_cycle, sim_cycle-event->reqAckTime); 
			}
			repl->ready = sim_cycle;

			if((event->prefetch_next == 2 || event->prefetch_next == 4)&& (event->operation == ACK_DIR_READ_SHARED || event->operation == ACK_DIR_READ_EXCLUSIVE))
			{
				repl->isL1prefetch = 1;
				repl->prefetch_time = sim_cycle;
			}
			else
			{
				repl->isL1prefetch = 0;
				repl->prefetch_time = 0;
			}

			if ((event->cp)->hsize)
				link_htab_ent((event->cp), &(event->cp)->sets[set], repl);

			/* Update the rs finish time in RS_LINK (event_queue) and mshr entry */

			if(!event->spec_mode && event->rs)
			{
				if((event->operation == ACK_DIR_READ_SHARED) || (event->operation == ACK_DIR_READ_EXCLUSIVE))
					eventq_update(event->rs, repl->ready);
				else
					event->rs->writewait = 2;
			}
			mshrentry_update(event, repl->ready, event->started);

			free_event(event);

			break;
#ifdef PREFETCH_OPTI
		case READ_DATA_REPLY_EXCLUSIVE:
		case READ_META_REPLY_EXCLUSIVE:
		case READ_DATA_REPLY_SHARED:
		case READ_META_REPLY_SHARED:
		case WRITE_DATA_REPLY:
		case WRITE_META_REPLY:
			dcache_access++;
			thecontexts[(event->des1-MEM_LOC_SHIFT)*mesh_size+event->des2]->dcache_access++;
			/* First find the replacement line */
			tag = CACHE_TAG(event->cp, addr);
			set = CACHE_SET(event->cp, addr);

			switch ((event->cp)->policy)
			{
				int bindex, i=0;
				case LRU:
				case FIFO:
				if(!mystricmp (network_type, "FSOI") || !mystricmp(network_type, "HYBRID"))
				{
#ifdef WRITE_EARLY
				i = event->cp->assoc;
				do {
					if(i == 0)	break;
					i--;
					repl = (event->cp)->sets[set].way_tail;
					update_way_list (&(event->cp)->sets[set], repl, Head);
				}
				while(repl->status & CACHE_BLK_WVALID);

				if(repl->status & CACHE_BLK_WVALID)
				{
					L1_flip_counter ++;
					event->when = sim_cycle + RETRY_TIME;
					dir_eventq_insert(event);
					return 1;
				}
#else
				repl = (event->cp)->sets[set].way_tail;
				update_way_list (&(event->cp)->sets[set], repl, Head);
#endif
				}
				else
				{
					repl = (event->cp)->sets[set].way_tail;
					update_way_list (&(event->cp)->sets[set], repl, Head);
				}
				break;
				case Random:
				bindex = myrand () & ((event->cp)->assoc - 1);
				repl = CACHE_BINDEX ((event->cp), (event->cp)->sets[set].blks, bindex);
				break;
				default:
				panic ("bogus replacement policy");
			}

			if ((repl->status & CACHE_BLK_VALID) 
#ifdef SILENT_DROP
#ifndef UPDATE_HINT
					&& (repl->status & CACHE_BLK_DIRTY)
#endif
#endif
			   )
			{
				if (collect_stats)
					event->cp->replacements++;

				if (actualProcess == 1 && !md_text_addr(repl->addr, tempID))     /*no write back to L2 for dual exec */
				{
					if ((repl->status & CACHE_BLK_DIRTY) && !(repl->state == MESI_MODIFIED))
						panic ("Cache Block should have been modified here");

					struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
					if(new_event == NULL) 	panic("Out of Virtual Memory");
					new_event->isPrefetch = 0;
#if defined(NON_SILENT_DROP) || defined(UPDATE_HINT)
					if (repl->status & CACHE_BLK_DIRTY)
					{
#endif
						if (collect_stats)
							event->cp->writebacks++;
#ifdef WB_SPLIT
						new_event->operation = WRITE_BACK_HEAD;
						packet_size = meta_packet_size;
						int j=0, wb_f;
						for(j=0;j<MSHR_SIZE;j++)
						{
							if(wb_buffer[event->des1*mesh_size+event->des2][j] == 0)
							{
								wb_buffer[event->des1*mesh_size+event->des2][j] = repl->addr;
								wb_f = 1;	
								break;
							}		
						}
						if(wb_f == 0)
							printf("Wb_addr pending buffer is full\n");
#else
						new_event->operation = WRITE_BACK;
						packet_size = data_packet_size;
#endif
#if defined(NON_SILENT_DROP) || defined(UPDATE_HINT)
					}
					else
					{
						new_event->operation = UPDATE_DIR;
						packet_size = meta_packet_size;
						if (collect_stats)
							event->cp->dir_notification++;
					} 
#endif
					new_event->conf_bit = -1;
					new_event->req_time = 0;
					new_event->input_buffer_full = 0;
					new_event->pending_flag = 0;
					new_event->op = event->op;
					new_event->src1 = event->des1;
					new_event->src2 = event->des2;
					md_addr_t src = (repl->addr >> cache_dl2->set_shift) % numcontexts;
					new_event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
					new_event->des2 = (src %mesh_size);
					new_event->processingDone = 0;
					new_event->startCycle = sim_cycle;
					new_event->parent = NULL;
					new_event->blk_dir = NULL;
					new_event->childCount = 0;
					new_event->tempID = event->tempID;
					new_event->L2miss_flag = event->L2miss_flag;
					new_event->resend = 0;
					new_event->blk1 = repl;
					new_event->addr = repl->addr;
					new_event->cp = event->cp;
					new_event->vp = NULL;
					new_event->nbytes = event->cp->bsize;
					new_event->udata = NULL;
					new_event->cmd = Write;
					new_event->rs = NULL; 
					new_event->spec_mode = 0;
					new_event->data_reply = 0;
					int new_vc = 0;
					/*go to network*/
					scheduleThroughNetwork(new_event, sim_cycle, packet_size, new_vc);

					dir_eventq_insert(new_event);
				}
			}
			/* update block tags */
			repl->tagid.tag = tag;
			repl->tagid.threadid = tempID;
			repl->addr = event->addr;
			repl->status = CACHE_BLK_VALID;           

			repl->blkImdtOp = 0;
			repl->blkNextOps = 0;

#ifdef SERIALIZATION_ACK	
			if(event->pendingInvAck)
				numStorePending[tempID]++;
#endif

			/* copy data out of cache block, if block exists */
			bofs = CACHE_BLK(event->cp, event->addr);
			if ((event->cp)->balloc && (event->vp))
				CACHE_BCOPY(event->cmd, repl, bofs, event->vp, event->nbytes);

			/* update cache block status at requester L1 */
			repl->status = CACHE_BLK_VALID;
			if(event->operation == READ_META_REPLY_EXCLUSIVE || event->operation == READ_DATA_REPLY_EXCLUSIVE) 
				repl->state = MESI_EXCLUSIVE;
			else if(event->operation == READ_META_REPLY_SHARED || event->operation == READ_DATA_REPLY_SHARED)
				repl->state = MESI_SHARED;
			else if(event->operation == WRITE_DATA_REPLY || event->operation == WRITE_META_REPLY) 
			{
				repl->state = MESI_MODIFIED;
				repl->status |= CACHE_BLK_DIRTY;
			}

			(event->cp)->last_tagsetid.tag = 0;
			(event->cp)->last_tagsetid.threadid = tempID;
			(event->cp)->last_blk = repl;

			/* get user block data, if requested and it exists */
			if (event->udata)
				*(event->udata) = repl->user_data;

			repl->ready = sim_cycle;

			if ((event->cp)->hsize)
				link_htab_ent((event->cp), &(event->cp)->sets[set], repl);

			/* Update the rs finish time in RS_LINK (event_queue) and mshr entry */

			if(!event->spec_mode && event->rs)
			{
				if((event->operation == READ_DATA_REPLY_SHARED) || (event->operation == READ_META_REPLY_SHARED) || (event->operation == READ_DATA_REPLY_EXCLUSIVE) || (event->operation == READ_META_REPLY_EXCLUSIVE))
					eventq_update(event->rs, repl->ready);
				else
					event->rs->writewait = 2;
			}
			mshrentry_update(event, repl->ready, event->started);

			free_event(event);
			break;
#endif
		case WAIT_MEM_READ	:
		case WAIT_MEM_READ_N	:
			{
				struct DIRECTORY_EVENT *p_event;
				struct DIRECTORY_EVENT *gp_event;
				if(event->operation == WAIT_MEM_READ_N)
					event->operation == WAIT_MEM_READ;
				int reFlag = 0;
				if(event->parent_operation < 100)
				{
					event->parent->childCount --;
					gp_event = event->parent;
					free_event(event);
					if(gp_event->childCount == cache_dl2->bsize/max_packet_size-1)
						reFlag = 1;
				}
				if(reFlag || event->parent_operation >= 100)
				{
					/* fixme: the parent event is wait memory model and parent's parent is the original request */
					if(reFlag)
					{
						if(gp_event->parent->operation == WAIT_MEM_READ)
						{
							p_event = gp_event->parent;
							//free_event(gp_event);
						}	
						else
							p_event = gp_event;
					}
					else if(event->parent_operation >= 100)
					{
						p_event = event;
						p_event->parent_operation = p_event->parent_operation - 100;
					}
					dcache2_access++;
					thecontexts[(p_event->des1-MEM_LOC_SHIFT)*mesh_size+p_event->des2]->dcache2_access++;
					p_event->L2miss_complete = sim_cycle;
					/* Wakeup from the L2 miss, then find the replacement cache line*/
					switch (cp_dir->policy)
					{
						int i;
						int bindex;
						case LRU:
						case FIFO:
						i = cp_dir->assoc;
						do {
							if(i == 0)	break;
							i--;
							repl = cp_dir->sets[set_dir].way_tail;
							update_way_list (&cp_dir->sets[set_dir], repl, Head);
						}
						while(repl_dir_state_check(repl->dir_state, (p_event->des1-MEM_LOC_SHIFT)*mesh_size+p_event->des2, repl->addr));

						if(repl_dir_state_check(repl->dir_state, (p_event->des1-MEM_LOC_SHIFT)*mesh_size+p_event->des2, repl->addr))
						{
							flip_counter ++;
							p_event->flip_flag = 1;
							p_event->when = sim_cycle + RETRY_TIME;
							p_event->parent_operation += 100;
							dir_eventq_insert(p_event);
							return 1;
						}
						break;
						case Random:
						bindex = myrand () & (cp_dir->assoc - 1);
						repl = CACHE_BINDEX (cp_dir, cp_dir->sets[set_dir].blks, bindex);
						break;
						default:
						panic ("bogus replacement policy");
					}

					repl->ever_dirty = 0;
					repl->ever_touch = 0;
					if (repl->status & CACHE_BLK_VALID)
					{  /* replace the replacement line in L2 cache */

						if (collect_stats)
							cp_dir->replacements++;

						int new_vc = 0;

						int i=0, m=0, iteration = 1, flag = 0, sharer_num = 0;
						for(i=0; i<(cache_dl2->set_shift - cache_dl1[0]->set_shift); i++)
							iteration = iteration * 2; 
						for (i=0; i<iteration; i++)
							if(Is_Shared(repl->dir_sharer[i], tempID))
								sharer_num ++;

						p_event->childCount = 0;				
						p_event->sharer_num = 0;

						for(i=0; i<iteration; i++)
						{
							if(Is_Shared(repl->dir_sharer[i], tempID))
							{  /* send invalidation messages to L1 cache of sharers*/
								/* Invalidate multiple sharers */
								for(m = 0; m<iteration; m++)
									repl->dir_state[m] = DIR_TRANSITION;
								repl->ptr_cur_event[i] = p_event;
								repl->addr = (repl->tagid.tag << cache_dl2->tag_shift) + (set_dir << cache_dl2->set_shift) + (i << cache_dl1[0]->set_shift);
								p_event->individual_childCount[i] = 0;
								for (Threadid = 0; Threadid < numcontexts; Threadid++)
								{
									if(((repl->dir_sharer[i][Threadid/64]) & ((unsigned long long int)1 << (Threadid%64))) == ((unsigned long long int)1 << (Threadid%64)))
									{
										if(collect_stats)
											cache_dl2->invalidations ++;
										struct DIRECTORY_EVENT *new_event = allocate_event(event->isSyncAccess);
										if(new_event == NULL)       panic("Out of Virtual Memory");
										involve_4_hops_miss ++;
										new_event->conf_bit = -1;
										new_event->req_time = 0;
										new_event->input_buffer_full = 0;
										new_event->pending_flag = 0;
										new_event->op = p_event->op;
										new_event->operation = INV_MSG_WRITE;
										new_event->isPrefetch = 0;
										new_event->tempID = p_event->tempID;
										new_event->L2miss_flag = p_event->L2miss_flag;
										new_event->resend = 0;
										new_event->blk1 = repl;
										new_event->addr = repl->addr;
										new_event->cp = p_event->cp;
										new_event->vp = NULL;
										new_event->nbytes = p_event->cp->bsize;
										new_event->udata = NULL;
										new_event->rs = NULL; 
										new_event->spec_mode = 0;
										new_event->src1 = p_event->des1;
										new_event->src2 = p_event->des2;
										new_event->des1 = Threadid/mesh_size+MEM_LOC_SHIFT;
										new_event->des2 = Threadid%mesh_size;
										new_event->processingDone = 0;
										new_event->startCycle = sim_cycle;
										new_event->parent = p_event;
										new_event->blk_dir = repl;
										p_event->sharer_num = sharer_num;
										p_event->blk_dir = repl;
										p_event->childCount++;
										p_event->individual_childCount[i] ++;
										if(!mystricmp (network_type, "FSOI") || !mystricmp(network_type, "HYBRID"))
										{
#ifdef INV_ACK_CON
										int thread_id;
										if(Is_Shared(repl->dir_sharer[i], tempID) > 1)
										{
											new_event->data_reply = 0;
											if((new_event->src1*mesh_size+new_event->src2 != new_event->des1*mesh_size+new_event->des2) && ((!mystricmp(network_type, "FSOI")) || (!mystricmp(network_type, "HYBRID") && ((abs(new_event->src1 - new_event->des1) + abs(new_event->src2 - new_event->des2)) > 1))))
											{
												new_event->data_reply = 0;
												struct DIRECTORY_EVENT *new_event2 = allocate_event(event->isSyncAccess);
												if(new_event2 == NULL)       panic("Out of Virtual Memory");
												involve_4_hops_miss ++;
												new_event2->conf_bit = -1;
												new_event2->req_time = 0;
												new_event2->input_buffer_full = 0;
												new_event2->pending_flag = 0;
												new_event2->op = p_event->op;
												new_event2->operation = ACK_MSG_WRITEUPDATE;
												new_event2->isPrefetch = 0;
												new_event2->tempID = p_event->tempID;
												new_event2->L2miss_flag = p_event->L2miss_flag;
												new_event2->resend = 0;
												new_event2->blk1 = repl;
												new_event2->addr = repl->addr;
												new_event2->cp = p_event->cp;
												new_event2->vp = NULL;
												new_event2->nbytes = p_event->cp->bsize;
												new_event2->udata = NULL;
												new_event2->rs = NULL; 
												new_event2->spec_mode = 0;
												new_event2->src1 = new_event->des1;
												new_event2->src2 = new_event->des2;
												new_event2->des1 = new_event->src1;
												new_event2->des2 = new_event->src2;
												new_event2->processingDone = 0;
												new_event2->startCycle = sim_cycle;
												new_event2->parent = p_event;
												new_event2->blk_dir = repl;
												new_event2->data_reply = 0;
												new_event2->when = WAIT_TIME;
												popnetMsgNo ++;
												new_event2->popnetMsgNo = popnetMsgNo;
												dir_eventq_insert(new_event2);
											}
											else	
												new_event->data_reply = 1;
										}
										else	
											new_event->data_reply = 1;
#else
										new_event->data_reply = 1;
#endif
										}
										else
											new_event->data_reply = 1;
										/*go to network*/
										scheduleThroughNetwork(new_event, sim_cycle, meta_packet_size, new_vc);

										dir_eventq_insert(new_event);
									}
								}
								flag = 1;
								sharer_distr[p_event->childCount] ++;
								IsNeighbor_sharer(repl->dir_sharer[i], tempID);
							}
						}
						if(flag == 1)
						{
							if(collect_stats)
								cp_dir->replInv++;
							return 1;
						}

						/* L2 write back latency */
						if(repl->status & CACHE_BLK_DIRTY)
						{
							int l=0;
							for(l=0;l<cache_dl2->bsize/max_packet_size;l++)
							{
								struct DIRECTORY_EVENT *new_event;
								new_event = allocate_new_event(p_event);
								new_event->addr = repl->addr;
								new_event->operation = MEM_WRITE_BACK;
								new_event->cmd = Write;
								scheduleThroughNetwork(new_event, sim_cycle, data_packet_size, new_vc);
								dir_eventq_insert(new_event);
							}	
							repl->Iswb_to_mem = 1;
							if(collect_stats)
								cache_dl2->writebacks ++;
						}
						else	
							repl->Iswb_to_mem = 0;
					}
					/* finish the memory read*/
					repl->dir_state[block_offset] = DIR_STABLE;
#ifdef PREFETCH_OPTI
					int i = 0;
					for(i=0;i<MAXTHREADS;i++)
						repl->dir_prefetch[block_offset][i] = 0;
					repl->dir_prefetch_num[block_offset] = 0;
					repl->prefetch_invalidate[block_offset] = 0;
					repl->prefetch_modified[block_offset] = 0;
#endif
					repl->ptr_cur_event[block_offset] = NULL;                                                                                                                                                                             
					repl->tagid.tag = tag_dir;
					repl->tagid.threadid = tempID;
					repl->addr = addr;
					repl->status = CACHE_BLK_VALID;
					repl->Iswb_to_mem = 0;
#ifdef STREAM_PREFETCHER
					repl->spTag = 0;
					if(dl2_prefetch_active)
						repl->spTag = dl2_prefetch_id + 1;
#endif

					/* jing: Here is a problem: copy data out of cache block, if block exists */
					/* do something here */

					if (repl->way_prev && cp_dir->policy == LRU)
						update_way_list(&cp_dir->sets[set_dir], repl, Head);

					cp_dir->last_tagsetid.tag = 0;
					cp_dir->last_tagsetid.threadid = tempID;
					cp_dir->last_blk = repl;
					/* My stats */
					repl->exclusive_time[block_offset] = sim_cycle;

					/* get user block data, if requested and it exists */
					if (p_event->udata)
						*(p_event->udata) = repl->user_data;

					repl->ready = sim_cycle;

					if (cp_dir->hsize)
						link_htab_ent(cp_dir, &cp_dir->sets[set_dir], repl);

					/* remove the miss entry in L2 MSHR*/
					update_L2_mshrentry(p_event->addr, sim_cycle, p_event->mshr_time);

					p_event->startCycle = sim_cycle;
					if(p_event->parent_operation == L2_PREFETCH)
					{
						repl->dir_dirty[block_offset] = -1;
						repl->dir_sharer[block_offset][0] = 0;
						repl->dir_sharer[block_offset][1] = 0;
						repl->dir_sharer[block_offset][2] = 0;
						repl->dir_sharer[block_offset][3] = 0;
						free_event(p_event);
						return 1;
					}

					if(p_event->parent_operation == MISS_IL1)
					{
						packet_size = data_packet_size;
						p_event->operation = ACK_DIR_IL1;
						repl->dir_dirty[block_offset] = -1;
						repl->dir_sharer[block_offset][0] = 0;
						repl->dir_sharer[block_offset][1] = 0;
						repl->dir_sharer[block_offset][2] = 0;
						repl->dir_sharer[block_offset][3] = 0;
						repl->dir_blk_state[block_offset] = MESI_SHARED;
						p_event->startCycle = sim_cycle;
					}
					else if(p_event->parent_operation == MISS_READ
#ifdef PREFETCH_OPTI
					|| p_event->parent_operation == L1_PREFETCH
					|| p_event->parent_operation == READ_COHERENCE_UPDATE
#endif
					)
					{
						packet_size = data_packet_size;
						p_event->operation = ACK_DIR_READ_EXCLUSIVE;
#ifdef LOCK_CACHE_REGISTER
						/* update the lock register if necessary */
						if(p_event->store_cond == MY_LDL_L)// && (bool_value)) //FIXME: check the value if bool or not
							LockRegesterCheck(p_event);
#endif				
						repl->dir_dirty[block_offset] = -1;
						if (!md_text_addr(addr, tempID))
						{
							repl->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
							repl->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
						}
						else
						{
							repl->dir_sharer[block_offset][0] = 0;
							repl->dir_sharer[block_offset][1] = 0;
							repl->dir_sharer[block_offset][2] = 0;
							repl->dir_sharer[block_offset][3] = 0;
							repl->dir_blk_state[block_offset] = MESI_SHARED;
						}
					}
					else if(p_event->parent_operation == MISS_WRITE
#ifdef PREFETCH_OPTI
					|| p_event->parent_operation == WRITE_COHERENCE_UPDATE
#endif
					)
					{
						packet_size = data_packet_size;
						p_event->operation = ACK_DIR_WRITE;
						repl->dir_dirty[block_offset] = tempID;
						repl->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
						repl->dir_blk_state[block_offset] = MESI_MODIFIED;
					}
					else
					{
						packet_size = meta_packet_size;
						p_event->operation = ACK_DIR_WRITEUPDATE;
						repl->dir_dirty[block_offset] = tempID;
						repl->dir_sharer[block_offset][tempID/64] = (unsigned long long int)1<<(tempID%64);
						repl->dir_blk_state[block_offset] = MESI_MODIFIED;
					}
					p_event->reqAckTime = sim_cycle;
					p_event->src1 = p_event->des1;
					p_event->src2 = p_event->des2;
					p_event->des1 = tempID/mesh_size+MEM_LOC_SHIFT;
					p_event->des2 = tempID%mesh_size;
					p_event->processingDone = 0;
					p_event->parent = NULL;
					p_event->blk_dir = repl;

					/*go to network*/
					scheduleThroughNetwork(p_event, p_event->startCycle, packet_size, vc);
					dir_eventq_insert(p_event);
					return 1;
				}
				else if(gp_event->childCount == 0)
				{
					//if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk, block_offset))
					//	blk->status_ll2[1-block_offset] = CACHE_BLK_VALID; //FIXME: hardcode here, only works with L2 blocksize twice than L1 blocksize
					//else
					//	second_mem_first ++;
					update_L2_mshrentry(gp_event->addr, sim_cycle, gp_event->mshr_time);
					free_event(gp_event);
				}
				break;
			}
		default			:
			panic("DIR: undefined directory operation");
	}
	return 1;
}

int repl_dir_state_check(int *dir_state, int id, md_addr_t addr)
{
	int m=0, iteration = 1, flag = 0;
#ifdef EUP_NETWORK
	if(!mystricmp (network_type, "FSOI") || (!mystricmp (network_type, "HYBRID")))
	{
	if(EUP_entry_replacecheck(id, addr>>cache_dl2->set_shift))
		return 1;
	}
#endif
	for(m=0; m<(cache_dl2->set_shift - cache_dl1[0]->set_shift); m++)
		iteration = iteration * 2; 
	for(m=0; m<iteration; m++)
		if(dir_state[m] == DIR_TRANSITION)
			flag = 1;
	return flag;
}

/********************************************multiprocessor************************************************/
/* access a cache, perform a CMD operation on cache CP at address ADDR,
   places NBYTES of data at *P, returns latency of operation if initiated
   at NOW, places pointer to block user data in *UDATA, *P is untouched if
   cache blocks are not allocated (!CP->BALLOC), UDATA should be NULL if no
   user data is attached to blocks */

extern totalUpgrades;
unsigned int			/* latency of access in cycles */
cache_access (struct cache_t *cp,	/* cache to access */
		enum mem_cmd cmd,	/* access type, Read or Write */
		md_addr_t addr,	/* address of access */
		void *vp,		/* ptr to buffer for input/output */
		int nbytes,	/* number of bytes to access */
		tick_t now,	/* time of access */
		byte_t ** udata,	/* for return of user data ptr */
		md_addr_t * repl_addr,	/* for address of replaced block */
		struct RUU_station *rs,
		int threadid
#ifdef SMD_USE_WRITE_BUF
		, m_L1WBufEntry *l1_wb_entry //8 equals thread id..
#endif
	      
)
{
	sprintf(dl1name, "dl1");

	if (!strcmp (cp->name, dl1name) && cp->threadid != threadid)
		panic ("Wrond DL1 cache being called in cache access\n");

	if(!strcmp (cp->name, dl1name) && md_text_addr(addr, threadid))
		return cache_dl1_lat;
	if(addr == 5368767520)
		printf("access here\n");
	byte_t *p = vp;
	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);
	md_addr_t bofs = CACHE_BLK (cp, addr);
	md_addr_t addr_prefetch;
	struct cache_blk_t *blk, *repl;
	int lat = 0, cnt;
	int vc = -1, a = 0;
	int isSyncAccess = 0;
	struct RUU_station  *my_rs = rs;


	md_addr_t wb_addr, replAddress;
	struct DIRECTORY_EVENT *event;

	int port_lat = 0, now_lat = 0, port_now = 0;
#if PROCESS_MODEL
	int isSharedAddress = 0;
#endif

#if PROCESS_MODEL
	/* Check is the data being read/written is in shared space or not */
	if (!strcmp (cp->name, dl1name))
	{
		isSharedAddress = checkForSharedAddr ((unsigned long long) addr);
	}
#endif
	/* Restart the thread that is in a quiescent state */
	/* This is true only for Data Cache */
	if (!strcmp (cp->name, dl1name))
	{
		if (cmd == Write)
		{
			for (cnt = 0; cnt < numcontexts; cnt++)
			{
				if ((quiesceAddrStruct[cnt].address == addr))
				{
					thecontexts[cnt]->freeze = 0;
					thecontexts[cnt]->running = 1;
					quiesceAddrStruct[cnt].address = 1;
					quiesceAddrStruct[cnt].threadid = -1;
				}
			}
		}
#ifdef  L1_STREAM_PREFETCHER
	/* Forward the request to stream prefetcher. do not prefetch if it's a sync instruction */
		if(rs)
		{
			if(!rs->isSyncInst)
				l1launch_sp (addr, threadid);
		}
		else
			l1launch_sp (addr, threadid);
#endif
	}

	/* default replacement address */
	if (repl_addr)
		*repl_addr = 0;
	/* check alignments */
	if ((nbytes & (nbytes - 1)) != 0 || (addr & (nbytes - 1)) != 0)
	{  
		printf("nbytes is %d\n", nbytes);
		fatal ("cache: access error: bad size or alignment, addr 0x%08x", addr);
	}
	/* access must fit in cache block */
	if ((addr + nbytes - 1) > ((addr & ~cp->blk_mask) + cp->bsize - 1))
		panic ("cache: access error: access spans block, addr 0x%08x", addr);

	if (strcmp (cp->name, dl1name))	//not level-1 data cache
	{
		if (cp->hsize)
		{
			/* higly-associativity cache, access through the per-set hash tables */
			int hindex = (int) CACHE_HASH (cp, tag);

			for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
			{
				if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
					goto cache_hit;
			}
		}
		else
		{
			/* low-associativity cache, linear search the way list */
			for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
			{
				if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
					goto cache_hit;
			}
		}
	}
	else
	{
		if(rs)
		{
			if(rs->isSyncInst)
			{
				SyncInstCacheAccess ++;
				if(my_rs->in_LSQ != 1)
					my_rs = &thecontexts[threadid]->LSQ[thecontexts[threadid]->LSQ_head];
				if(!my_rs)
					panic("lsq does not have any data\n");	
				if(my_rs->op == LW)
					TestCacheAccess ++;
				else if(my_rs->op == LL)
					TestSecCacheAccess ++;
				else if(my_rs->op == SC)
					SetCacheAccess ++;
			}
#ifdef BAR_OPT
			if(rs->isSyncInstBarRel)
			{
				if(my_rs->in_LSQ != 1)
					my_rs = &thecontexts[threadid]->LSQ[thecontexts[threadid]->LSQ_head];
				if(!my_rs)
					panic("lsq does not have any data\n");	
			}
#endif
		}
#ifdef SMD_USE_WRITE_BUF
		else if(l1_wb_entry)
			if(l1_wb_entry->op == SC)
				SetCacheAccess ++;
#endif
		switch (cmd)
		{
			case Read:
				for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
				{
					if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID) && ((blk->state == MESI_SHARED) || (blk->state == MESI_MODIFIED) || (blk->state == MESI_EXCLUSIVE)))
					{
						blk->addr = addr;
						/* These statistics are for the coherent cache */
						blk->ReadCount ++;
						blk->WordUseFlag[(blk->addr>>2) & 7] = 1;
						if (collect_stats)
						{
							switch (blk->state)
							{
								case MESI_SHARED:
									cp->ccLocalLoadS++;
									break;
								case MESI_EXCLUSIVE:
									cp->ccLocalLoadX++;
									break;
								case MESI_MODIFIED:
									cp->ccLocalLoadM++;
									break;
								default:
									break;
							}
						}
#ifdef PREFETCH_OPTI
						if(blk->isL1prefetch)
						{
							L1_prefetch_usefull ++;
							ReadPrefetchUpdate ++;
							//if(blk->prefetch_time)
							//	fprintf(fp_trace, "%d\n", sim_cycle - blk->prefetch_time);
							blk->isL1prefetch = 0;
							blk->prefetch_time = 0;
							blk->status = 0;  //make the block invalid, and keep the data at the MSHR
							/* create the new event sending out to directory to update the cache coherence information */
							event = (struct DIRECTORY_EVENT *) allocate_event(isSyncAccess); //create a space
							if (!event)
								fatal("out of virtual memory in calloc dir event");

							event->op = 0; 
							event->isPrefetch = 0;
							event->isSyncAccess = 0;
							event->spec_mode = 0;
							event->src1 = thecontexts[threadid]->actualid/mesh_size+MEM_LOC_SHIFT;
							event->src2 = thecontexts[threadid]->actualid%mesh_size;
							int src = (addr >> cache_dl2->set_shift) % numcontexts;
							event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
							event->des2 = (src %mesh_size);
							event->started = now;
							event->startCycle = now;
							event->pending_flag = 0;
							event->data_reply = 0;
							event->req_time = 0;
							missNo ++;
							event->resend = 0;
							event->childCount = 0;
							event->parent = NULL;
							event->operation = READ_COHERENCE_UPDATE;
							event->tempID = threadid;
							event->blk1 = NULL;
							event->addr = addr;
							event->cp = cp;
							event->vp = p;
							event->nbytes = nbytes;
							event->udata = udata;
							event->cmd = cmd;
							event->rs = rs; 
#ifdef SMD_USE_WRITE_BUF
							event->l1_wb_entry = l1_wb_entry;
#endif
							event->missNo = missNo; 
							event->l2Status = 0;
							event->l2MissStart = 0;
							event->L2miss_flag = 0;
							event->sharer_num = 0;
							event->dirty_flag = 0;
							event->arrival_time = 0;
							event->conf_bit = -1;
							event->input_buffer_full = 0;

							scheduleThroughNetwork(event, now, meta_packet_size, vc);
							dir_eventq_insert(event);
							event_created = event;
							return WAIT_TIME;
						}
						else
							goto cache_hit;
#else						
						goto cache_hit;
#endif
					}
				}
				break;
			case Write:
				for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
				{
					if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
					{
						blk->addr = addr;
						switch (blk->state)
						{
							case MESI_MODIFIED:
								if (collect_stats)
									cp->ccLocalStoreM++;

								blk->WriteCount ++;
								blk->WordUseFlag[(blk->addr>>2) & 7] = 1;
								goto cache_hit;
								break;

							case MESI_SHARED:
								if (collect_stats)
									cp->ccLocalStoreS++;
								if (spec_benchmarks)
									panic ("The state can never be shared \n");

								blk->status = 0; /* We are not modeling data here. For proper data modeling we need to keep this block's data saved in MSHR. */
								if(collect_stats)
								{
									/* collect some stats */
									if(blk->blkAlocReason == LINE_PREFETCH)
									{
										if(blk->blkImdtOp)
											totalL1LinePrefUse++;
										else
											totalL1LinePrefNoUse++;
									}
									else if(blk->blkAlocReason == LINE_READ)
									{
										if(blk->blkImdtOp)
											totalL1LineReadUse++;
										else
											totalL1LineReadNoUse++;
									}
									else if (blk->blkAlocReason == LINE_WRITE)
									{
										if(blk->blkImdtOp)
											totalL1LineWriteUse++;
										else
											totalL1LineWriteNoUse++;
									}
								}
								if(rs)
									isSyncAccess = rs->isSyncInst;
#ifdef SMD_USE_WRITE_BUF
								else if(l1_wb_entry)
									if(l1_wb_entry->op == SC)
										isSyncAccess = 1;
#endif
								event = (struct DIRECTORY_EVENT *)
									allocate_event(isSyncAccess); //create a space                                                                                                                                                                             
								if (!event)
									fatal("out of virtual memory in calloc dir event");

								long src;
								if(my_rs)
								{
									event->op = my_rs->op; 
									event->isPrefetch = !my_rs->issued;
									event->isSyncAccess = isSyncAccess;
									event->spec_mode = my_rs->spec_mode;
									if(my_rs->isSyncInst)
									{
										if(my_rs->op == SC || my_rs->op == SC)
										{
											event->store_cond = MY_STL_C;
											SyncStoreCWriteUpgrade ++;	
										}
										if(my_rs->op == SW)
										{
											SyncStoreWriteUpgrade ++;	
											event->store_cond = MY_STL;
										}
									}
#ifdef BAR_OPT
									if(my_rs->isSyncInstBarRel && my_rs->op == SW)
									{
										BarStoreWriteUpgrade ++;	
										event->store_cond = MY_STL;
									}
#endif
								}
#ifdef SMD_USE_WRITE_BUF
								else if(l1_wb_entry)
								{
									if(l1_wb_entry->op == SC)
									{
										event->store_cond = MY_STL_C;
										SyncStoreCWriteUpgrade ++;	
									}
								}
#endif
								else
								{
									event->op = 0; 
									event->isPrefetch = 0;
									event->isSyncAccess = 0;
									event->spec_mode = 0;
								}
								event->src1 = thecontexts[threadid]->actualid/mesh_size+MEM_LOC_SHIFT;
								event->src2 = thecontexts[threadid]->actualid%mesh_size;
#ifdef CENTRALIZED_L2
								//the centralized L2 has CENTL2_BANK_NUM banks, and each of them has one NI (network interface). 
								src = (addr >> cache_dl2->set_shift) % CENTL2_BANK_NUM;
								event->des1 = mesh_size + 2 ;    //This indicates that its a L2 bank access, the location of this L2 bank NI is on the bottom of the chip, (In fact, it can be anywhere) 
								event->des2 = src; //this indicate the bank id
#else
								src = (addr >> cache_dl2->set_shift) % numcontexts;
								event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
								event->des2 = (src %mesh_size);
#endif
								event->started = now;
								event->startCycle = now;
								event->pending_flag = 0;
								event->data_reply = 0;
								event->req_time = 0;

								if(collect_stats)
								{
									cp->invalidations++;
									if(((event->addr)>>cache_dl2->set_shift)%numcontexts == thecontexts[threadid]->actualid)
										local_cache_access++;
									else
										remote_cache_access++;
								}

								missNo ++;
								event->resend = 0;
								event->childCount = 0;
								event->parent = NULL;
								event->operation = WRITE_UPDATE;
								m_update_miss[threadid] = 1;
								
								if(collect_stats)
									totaleventcountnum ++;
								if(collect_stats && event->isSyncAccess)
								{
									totalSyncWriteup ++;
									totalSyncEvent ++;
								}
								else
									totalNormalEvent ++;
								totalUpgrades++;
								event->tempID = threadid;
								event->blk1 = NULL;
								event->addr = addr;
								event->cp = cp;
								event->vp = p;
								event->nbytes = nbytes;
								event->udata = udata;
								event->cmd = cmd;
								event->rs = rs; 
#ifdef SMD_USE_WRITE_BUF
								event->l1_wb_entry = l1_wb_entry;
#endif
								event->missNo = missNo; 
								event->l2Status = 0;
								event->l2MissStart = 0;
								event->L2miss_flag = 0;
								event->sharer_num = 0;
								event->dirty_flag = 0;
								event->arrival_time = 0;
								event->conf_bit = -1;
								event->input_buffer_full = 0;

								scheduleThroughNetwork(event, now, meta_packet_size, vc);
								dir_eventq_insert(event);//insert the directory event of write hit

								event_created = event;
								if(blk->isL1prefetch)
									L1_prefetch_writeafter ++;

								lat = lat + WAIT_TIME;                                                                                                                                                            
								return lat;
								break;

							case MESI_EXCLUSIVE:
								if (collect_stats)
									cp->e_to_m++;
								e_to_m ++;
								totalL1LineExlUsed++;
								blk->state = MESI_MODIFIED;
								blk->WriteCount ++;
								blk->WordUseFlag[(blk->addr>>2) & 7] = 1;
#ifdef COHERENCE_MODIFIED
								event = (struct DIRECTORY_EVENT *) allocate_event(isSyncAccess); //create a space
								if (!event)
									fatal("out of virtual memory in calloc dir event");

								event->op = 0; 
								event->isPrefetch = 0;
								event->isSyncAccess = 0;
								event->spec_mode = 0;
								event->src1 = thecontexts[threadid]->actualid/mesh_size+MEM_LOC_SHIFT;
								event->src2 = thecontexts[threadid]->actualid%mesh_size;
								src = (addr >> cache_dl2->set_shift) % numcontexts;
								event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
								event->des2 = (src %mesh_size);
								event->started = now;
								event->startCycle = now;
								event->pending_flag = 0;
								event->data_reply = 0;
								event->req_time = 0;
								missNo ++;
								event->resend = 0;
								event->childCount = 0;
								event->parent = NULL;
								event->operation = WRITE_INDICATE;
								
								if(collect_stats)
									totalWriteIndicate ++;
								event->tempID = threadid;
								event->blk1 = NULL;
								event->addr = addr;
								event->cp = cp;
								event->vp = p;
								event->nbytes = nbytes;
								event->udata = udata;
								event->cmd = cmd;
								event->rs = rs; 
#ifdef SMD_USE_WRITE_BUF
								event->l1_wb_entry = l1_wb_entry;
#endif
								event->missNo = missNo; 
								event->l2Status = 0;
								event->l2MissStart = 0;
								event->L2miss_flag = 0;
								event->sharer_num = 0;
								event->dirty_flag = 0;
								event->arrival_time = 0;
								event->conf_bit = -1;
								event->input_buffer_full = 0;

								scheduleThroughNetwork(event, now, meta_packet_size, vc);
								dir_eventq_insert(event);//insert the directory event of write hit
								event_created = event;
#endif
#ifdef PREFETCH_OPTI
								if(blk->isL1prefetch)
								{
									L1_prefetch_usefull ++;
									L1_prefetch_writeafter ++;
									WritePrefetchUpdate ++;
									//if(blk->prefetch_time)
									//	fprintf(fp_trace, "%d\n", sim_cycle - blk->prefetch_time);
									blk->isL1prefetch = 0;
									blk->prefetch_time = 0;
									blk->status = 0;
									/* create the new event sending out to directory to update the cache coherence information */
									event = (struct DIRECTORY_EVENT *) allocate_event(isSyncAccess); //create a space
									if (!event)
										fatal("out of virtual memory in calloc dir event");

									event->op = 0; 
									event->isPrefetch = 0;
									event->isSyncAccess = 0;
									event->spec_mode = 0;
									event->src1 = thecontexts[threadid]->actualid/mesh_size+MEM_LOC_SHIFT;
									event->src2 = thecontexts[threadid]->actualid%mesh_size;
									src = (addr >> cache_dl2->set_shift) % numcontexts;
									event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
									event->des2 = (src %mesh_size);
									event->started = now;
									event->startCycle = now;
									event->pending_flag = 0;
									event->data_reply = 0;
									event->req_time = 0;
									missNo ++;
									event->resend = 0;
									event->childCount = 0;
									event->parent = NULL;
									event->operation = WRITE_COHERENCE_UPDATE;
									event->tempID = threadid;
									event->blk1 = NULL;
									event->addr = addr;
									event->cp = cp;
									event->vp = p;
									event->nbytes = nbytes;
									event->udata = udata;
									event->cmd = cmd;
									event->rs = rs; 
#ifdef SMD_USE_WRITE_BUF
									event->l1_wb_entry = l1_wb_entry;
#endif
									event->missNo = missNo; 
									event->l2Status = 0;
									event->l2MissStart = 0;
									event->L2miss_flag = 0;
									event->sharer_num = 0;
									event->dirty_flag = 0;
									event->arrival_time = 0;
									event->conf_bit = -1;
									event->input_buffer_full = 0;

									scheduleThroughNetwork(event, now, meta_packet_size, vc);
									dir_eventq_insert(event);
									event_created = event;
									return WAIT_TIME;
								}
								else
									goto cache_hit;
#else						
								goto cache_hit;
#endif
								break;
							default:
								panic ("Should not be invalid");
								break;
						}
					}
				}
				break;
			default:
				panic ("Cache to the command is neither read nor write");
				break;
		}
	}

	int flag = 0, next_block = 0, pre_block = 0;
	if(!strcmp(cp->name, dl1name))
	{/* This is a L1 miss, we are going to create an event for L2 director access. */

		if(collect_stats)
		{
			for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
			{
				if ((blk->tagid.tag == tag) && (blk->status == 0)&& (blk->state == MESI_INVALID))
				{
					cp->coherence_misses ++;
					flag = 1;
				}
			}
			if(flag != 1)
				cp->capacitance_misses ++;
		}
#ifdef DL1_PREFETCH
		/* find if next block is in the cache line */
		if(set == 0)
		{
			for (blk = cp->sets[cp->nsets-1].way_head; blk; blk = blk->way_next)
			{
				if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
					pre_block = 1;
			}
		}
		else
		{
			for (blk = cp->sets[(set-1)%cp->nsets].way_head; blk; blk = blk->way_next)
			{
				if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
					pre_block = 1;
			}
		}
		for (blk = cp->sets[(set+1)%cp->nsets].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
				next_block = 1;
		}
#endif

		if(rs)
			isSyncAccess = rs->isSyncInst;
#ifdef SMD_USE_WRITE_BUF
		else if(l1_wb_entry)
			if(l1_wb_entry->op == SC)
			isSyncAccess = 1;
#endif
		event = (struct DIRECTORY_EVENT *)
			allocate_event(isSyncAccess); //create a space

		if (!event)
			fatal("out of virtual memory in calloc dir event");
		if (cmd == Read)
		{
			event->operation = MISS_READ;
			totalmisscountnum ++;
			event->isPrefetch = rs->isPrefetch;
		}
		else
		{
			event->operation = MISS_WRITE;
			totalreqcountnum ++;
			if(rs)
				event->isPrefetch = !rs->issued;
			else
				event->isPrefetch = 0;
		}
		if(collect_stats)
			totaleventcountnum ++;
		if(collect_stats && isSyncAccess)
		{
			totalSyncEvent ++;
			if(event->operation == MISS_WRITE)
				totalSyncWriteM ++;
			else
				totalSyncReadM ++;
		}
		else
			totalNormalEvent ++;
		missNo ++;                                                                                                                                                                                     
		long src;
		event->pending_flag = 0;
		//event->op = LDQ;
		event->op = LW;//wxh
		event->src1 = thecontexts[threadid]->actualid/mesh_size+MEM_LOC_SHIFT;
		event->src2 = thecontexts[threadid]->actualid%mesh_size;
#ifdef CENTRALIZED_L2
		//the centralized L2 has CENTL2_BANK_NUM banks, and each of them has one NI (network interface). 
		src = (addr >> cache_dl2->set_shift) % CENTL2_BANK_NUM;
		event->des1 = mesh_size + 2 ;    //This indicates that its a L2 bank access, the location of this L2 bank NI is on the bottom of the chip, (In fact, it can be anywhere) 
		event->des2 = src; //this indicate the bank id
#else
		src = (addr >> cache_dl2->set_shift) % numcontexts;
		event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
		event->des2 = (src %mesh_size);
#endif
		event->started = now;
		event->startCycle = now;
		if(rs)
		{
			event->isSyncAccess = isSyncAccess;
			event->spec_mode = rs->spec_mode;
		}
#ifdef SMD_USE_WRITE_BUF
		else if(l1_wb_entry)
		{
			if(l1_wb_entry->op == SC)
			{
				event->isSyncAccess = isSyncAccess;
				event->spec_mode = 0;
			}
		}
#endif
		else
		{
			event->isSyncAccess = 0;
			event->spec_mode = 0;
		}

		if(collect_stats)
		{	
			cp->misses ++;
			if((addr>>cache_dl2->set_shift)%numcontexts == thecontexts[threadid]->actualid)
				local_cache_access++;
			else
				remote_cache_access++;
		}

		event->childCount = 0;
		event->parent = NULL;
		event->l2Status = 0;
		event->l2MissStart = 0;
		event->tempID = threadid;
		event->resend = 0;
		event->blk1 = NULL;
		event->addr = addr;
		event->cp = cp;
		event->vp = p;
		event->nbytes = nbytes;
		event->udata = udata;
		event->cmd = cmd;
		event->rs = rs; 
#ifdef SMD_USE_WRITE_BUF
		event->l1_wb_entry = l1_wb_entry;
#endif
		event->missNo = missNo; 
		event->L2miss_flag = 0;
		event->sharer_num = 0;
		event->prefetch_next = 0;
		event->dirty_flag = 0;
		event->arrival_time = 0;
		event->conf_bit = -1;
		event->data_reply = 0;
		event->req_time = 0;
		if(my_rs)
		{
			if(my_rs->isSyncInst && my_rs->op == LW)
			{
				SyncLoadReadMiss ++;
				event->store_cond = MY_LDL;
			}
			if(my_rs->op == LL)
			{
				SyncLoadLReadMiss ++;
				event->store_cond = MY_LDL_L;
			}
			if(my_rs->op == SC)
			{
				SyncStoreCWriteMiss ++;
				event->store_cond = MY_STL_C;
			}
			if(my_rs->isSyncInst && my_rs->op == SW)
			{
				SyncStoreWriteMiss ++;
				event->store_cond = MY_STL;
			}
#ifdef BAR_OPT
			if(my_rs->isSyncInstBarRel && my_rs->op == SW)
			{
				event->store_cond = MY_STL;
				BarStoreWriteMiss ++;
			}
#endif
		}
#ifdef SMD_USE_WRITE_BUF
		else if(l1_wb_entry)
		{
			if(l1_wb_entry->op == SC)
			{
				SyncStoreCWriteMiss ++;
				event->store_cond = MY_STL_C;
			}
		}
#endif
	

#ifdef  L1_STREAM_PREFETCHER 
		/* L1 cache miss is detected, insert this address into stream pre-fetcher. */
		if (!dl1_prefetch_active)
		{
			if(rs)
			{
				if(!rs->isSyncInst)
					l1insert_sp (addr, threadid);
			}
			else
				l1insert_sp (addr, threadid);
		}
#endif  
#ifdef DL1_PREFETCH
		if(!next_block && pre_block)
		{
			addr_prefetch = (tag << cp->tag_shift) + (((set+1)%cp->nsets) << cp->set_shift);
			src = (addr_prefetch >> cache_dl2->set_shift) % numcontexts;
			totaleventcountnum ++;
			total_L1_prefetch ++;	   

			int matchnum;
			matchnum = MSHR_block_check(cp->mshr, addr_prefetch, cp->set_shift);
			if(!matchnum && (cp->mshr->freeEntries > 1))
			{
				struct DIRECTORY_EVENT *new_event = allocate_event(0);
				if(new_event == NULL)       panic("Out of Virtual Memory");
				new_event->data_reply = 0;
				new_event->req_time = 0;
				new_event->pending_flag = 0;
				new_event->op = event->op;
				new_event->isPrefetch = 0;
#ifdef PREFETCH_OPTI
				new_event->operation = L1_PREFETCH;
#else
				new_event->operation = MISS_READ;
#endif
				new_event->src1 = event->src1;
				new_event->src2 = event->src2;
#ifdef CENTRALIZED_L2
				//the centralized L2 has CENTL2_BANK_NUM banks, and each of them has one NI (network interface). 
				src = (addr_prefetch >> cache_dl2->set_shift) % CENTL2_BANK_NUM;
				new_event->des1 = mesh_size + 2 ;    //This indicates that its a L2 bank access, the location of this L2 bank NI is on the bottom of the chip, (In fact, it can be anywhere) 
				new_event->des2 = src; //this indicate the bank id
#else
				new_event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
				new_event->des2 = (src %mesh_size);
#endif
				new_event->processingDone = 0;
				new_event->startCycle = sim_cycle;
				new_event->parent = NULL;
				new_event->tempID = event->tempID;
				new_event->resend = 0;
				new_event->blk_dir = NULL;
				new_event->cp = event->cp;
				new_event->addr = addr_prefetch;
				new_event->vp = NULL;
				new_event->nbytes = event->cp->bsize;
				new_event->udata = NULL;
				new_event->cmd = Write;
				new_event->rs = NULL;
#ifdef SMD_USE_WRITE_BUF
				new_event->l1_wb_entry = NULL;
#endif
				new_event->started = event->started;
				new_event->spec_mode = 0;
				new_event->L2miss_flag = 0;
				new_event->prefetch_next = 2;	
				new_event->conf_bit = -1;
				new_event->input_buffer_full = 0;

				scheduleThroughNetwork(new_event, now, meta_packet_size, vc);

				dir_eventq_insert(new_event);
				event_created = new_event;
				MSHRLookup(cp->mshr, addr_prefetch, WAIT_TIME, 0, rs);
			}
		}
#endif

		event_created = event;
		event->input_buffer_full = 0;
		scheduleThroughNetwork(event, now, meta_packet_size, vc);

		dir_eventq_insert(event);
		return WAIT_TIME;
	}
	else if(!strcmp (cp->name, "il1"))
	{
		event = (struct DIRECTORY_EVENT *)
			allocate_event(0); //create a space

		if (!event)
			fatal("out of virtual memory in calloc dir event");
		if(cmd == Write)
			panic("Il1 access could-not lead to Write command");

		long src;
		//event->op = LDQ;
		event->op = LW; //wxh
		event->pending_flag = 0;
		event->operation = MISS_IL1;
		event->isPrefetch = 0;
		event->src1 = thecontexts[threadid]->actualid/mesh_size+MEM_LOC_SHIFT;
		event->src2 = thecontexts[threadid]->actualid%mesh_size;
#ifdef CENTRALIZED_L2
		//the centralized L2 has CENTL2_BANK_NUM banks, and each of them has one NI (network interface). 
		src = (addr >> cache_dl2->set_shift) % CENTL2_BANK_NUM;
		event->des1 = mesh_size + 2 ;    //This indicates that its a L2 bank access, the location of this L2 bank NI is on the bottom of the chip, (In fact, it can be anywhere) 
		event->des2 = src; //this indicate the bank id
#else
		src = (addr >> cache_dl2->set_shift) % numcontexts;
		event->des1 = (src /mesh_size)+MEM_LOC_SHIFT;
		event->des2 = (src %mesh_size);
#endif
		event->started = now;
		event->startCycle = now;
		event->data_reply = 0;
		event->req_time = 0;

		if(collect_stats)
		{
			cp->misses ++;
			if((addr>>cache_dl2->set_shift)%numcontexts == thecontexts[threadid]->actualid)
				local_cache_access++;
			else
				remote_cache_access++;
		}

		event->childCount = 0;
		event->parent = NULL;
		event->l2Status = 0;
		event->l2MissStart = 0;
		event->tempID = threadid;
		event->resend = 0;
		event->blk1 = NULL;
		event->addr = addr;
		event->cp = cp;
		event->vp = p;
		event->nbytes = nbytes;
		event->udata = udata;
		event->cmd = cmd;
		event->rs = NULL; 
#ifdef SMD_USE_WRITE_BUF
		event->l1_wb_entry = l1_wb_entry;
#endif
		event->missNo = missNo; 
		event->spec_mode = thecontexts[threadid]->spec_mode;
		event->L2miss_flag = 0;
		event->sharer_num = 0;
		event->dirty_flag = 0;
		event->arrival_time = 0;
		event->conf_bit = -1;
		event->input_buffer_full = 0;

		scheduleThroughNetwork(event, now, meta_packet_size, vc);

		dir_eventq_insert(event);
		event_created = event;
		thecontexts[threadid]->event = event;
		return WAIT_TIME;
	}


	/* select the appropriate block to replace, and re-link this entry to
	   the appropriate place in the way list */
	switch (cp->policy)
	{
		case LRU:
		case FIFO:
			repl = cp->sets[set].way_tail;
			update_way_list (&cp->sets[set], repl, Head);
			break;
		case Random:
			{
				int bindex = myrand () & (cp->assoc - 1);

				repl = CACHE_BINDEX (cp, cp->sets[set].blks, bindex);
				break;
			}
		default:
			panic ("bogus replacement policy");
	}


	if (strcmp (cp->name, dl1name))	//not level-1 data cache
	{
		/* **MISS** */
		if (collect_stats)
			cp->misses++;

		/* remove this block from the hash bucket chain, if hash exists */
		if (cp->hsize)
			unlink_htab_ent (cp, &cp->sets[set], repl);

		/* blow away the last block to hit */
		cp->last_tagsetid.tag = 0;
		cp->last_tagsetid.threadid = -1;
		cp->last_blk = NULL;

		lat += cp->hit_latency;

		/* write back replaced block data */
		if (repl->status & CACHE_BLK_VALID)
		{
			if (collect_stats)
				cp->replacements++;

			if (repl_addr)
				*repl_addr = CACHE_MK_BADDR (cp, repl->tagid.tag, set);

			/* don't replace the block until outstanding misses are satisfied */
			lat += BOUND_POS (repl->ready - now);

			if ((repl->status & CACHE_BLK_DIRTY))
			{
				/* write back the cache block */
				if (collect_stats)
				{
					cp->writebacks++;
					cp->wrb++;
				}
				lat += cp->blk_access_fn (Write, CACHE_MK_BADDR (cp, repl->tagid.tag, set), cp->bsize, repl, now + lat, repl->tagid.threadid);
			}
		}

		/* update block tags */
		repl->tagid.tag = tag;
		repl->tagid.threadid = threadid;
		repl->status = CACHE_BLK_VALID;	/* dirty bit set on update */
		repl->addr = addr;
		repl->lineVolatile = 0;
		repl->wordVolatile = 0;
		repl->wordInvalid = 0;
		repl->isStale = 0;

		/* read data block */
		lat += cp->blk_access_fn (Read, CACHE_BADDR (cp, addr), cp->bsize, repl, now + lat, threadid);

		/* copy data out of cache block */
		if (cp->balloc && p)
		{
			CACHE_BCOPY (cmd, repl, bofs, p, nbytes);
		}

		/* update dirty status */
		if (cmd == Write)
		{
			repl->status |= CACHE_BLK_DIRTY;
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

		/* update block status */
		repl->ready = now + lat;
		/* link this entry back into the hash table */
		if (cp->hsize)
			link_htab_ent (cp, &cp->sets[set], repl);

		/* return latency of the operation */
		return lat;
	}
	else			/*DL1 cache */
	{
		panic(" DL1 access could not ENTER this part of the code ");
	}				/* MSI and DL1 cache */

cache_hit:			/* slow hit handler */

	/* **HIT** */
	if (collect_stats)
		cp->hits++;


	/* copy data out of cache block, if block exists */
	if (cp->balloc && p)
	{
		CACHE_BCOPY (cmd, blk, bofs, p, nbytes);
	}

	/* update dirty status */
	if (cmd == Write)
		blk->status |= CACHE_BLK_DIRTY;

	blk->isStale = 0;

	/* Update access stats - Ronz */
	if (!strcmp (cp->name, "dl1"))
	{
		if (cmd == Write)
			wh1++;
		else
			rh1++;

		if(blk->blkImdtOp)
			blk->blkNextOps = ((cmd == Write) ? LINE_WRITE_OP : LINE_READ_OP);
		else
			blk->blkImdtOp = (cmd == Write) ? LINE_WRITE_OP : LINE_READ_OP;

		if(blk->isL1prefetch)
		{
			L1_prefetch_usefull ++;
			//if(blk->prefetch_time)
			//	fprintf(fp_trace, "%d\n", sim_cycle - blk->prefetch_time);
		}
		blk->isL1prefetch = 0;
		blk->prefetch_time = 0;
		if(my_rs)
		{
			if(my_rs->isSyncInst && my_rs->op == LW)
				SyncLoadHit ++;
			if(my_rs->op == LL)
				SyncLoadLHit ++;	
			if(my_rs->op == SC)
				SyncStoreCHit ++;
			if(my_rs->isSyncInst && my_rs->op == SW)
				SyncStoreHit ++;
		}
#ifdef SMD_USE_WRITE_BUF
		else if(l1_wb_entry)
			if(l1_wb_entry->op == SC)
				SyncStoreCHit ++;
#endif
	}
	if (!strcmp (cp->name, "ul2"))
	{
		if (cmd == Write)
			wh2++;
		else
			rh2++;
	}
#ifdef STREAM_PREFETCHER
	if (!strcmp (cp->name, "ul2") && blk->spTag && !dl2_prefetch_active)
	{
		launch_sp(blk->spTag-1, threadid);
	}
	blk->spTag = 0;
#endif

	if (blk->way_prev && cp->policy == LRU)
	{
		update_way_list (&cp->sets[set], blk, Head);
	}

	/* tag is unchanged, so hash links (if they exist) are still valid */

	/* record the last block to hit */
	cp->last_tagsetid.tag = CACHE_TAGSET (cp, addr);
	cp->last_tagsetid.threadid = threadid;
	cp->last_blk = blk;

	/* get user block data, if requested and it exists */
	if (udata)
		*udata = blk->user_data;

	if((int) max2 ((cp->hit_latency + lat), (blk->ready - now)) > WAIT_TIME)
		panic("Cache Hit latency too large\n");

	/* return first cycle data is available to access */
	return (int) max2 ((cp->hit_latency + lat), (blk->ready - now));	//add the invalidation latency

}				/* cache_access */

void cache_warmup (struct cache_t *cp,	/* cache to access */
		enum mem_cmd cmd,		/* access type, Read or Write */
		md_addr_t addr,		/* address of access */
		int nbytes,		/* number of bytes to access */
		int threadid)
{
	/* FIXME: During warmup we bring the data in shared mode even during writes. */
	return;

	sprintf(dl1name, "dl1");

	if(!strcmp (cp->name, dl1name) && cp->threadid != threadid)
		panic ("Wrond DL1 cache being called in cache access\n");
	if(!strcmp (cp->name, dl1name) && md_text_addr(addr, threadid))
		return;

	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);

	struct cache_blk_t *blk, *repl, *repl1;

#if PROCESS_MODEL
	int isSharedAddress = 0;
#endif

#if PROCESS_MODEL
	/* Check is the data being read/written is in shared space or not */
	if (!strcmp (cp->name, dl1name))
	{
		isSharedAddress = checkForSharedAddr ((unsigned long long) addr);
	}
#endif

	/* check alignments */
	if ((nbytes & (nbytes - 1)) != 0 || (addr & (nbytes - 1)) != 0)
	{  
		printf("nbytes is %d\n", nbytes);
		fatal ("cache: access error: bad size or alignment, addr 0x%08x", addr);
	}

	/* access must fit in cache block */
	if ((addr + nbytes - 1) > ((addr & ~cp->blk_mask) + cp->bsize - 1))
		panic ("cache: access error: access spans block, addr 0x%08x", addr);

	if (cp->hsize)
	{
		/* higly-associativity cache, access through the per-set hash tables */
		int hindex = (int) CACHE_HASH (cp, tag);

		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
			{
				if (blk->way_prev && cp->policy == LRU)
					update_way_list(&cp->sets[set], blk, Head);
				return;
			}
		}
	}
	else
	{
		/* low-associativity cache, linear search the way list */
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
			{
				if (blk->way_prev && cp->policy == LRU)
					update_way_list(&cp->sets[set], blk, Head);
				return;
			}
		}
	}

	/* Miss in L1 cache, first search the L2 cache */
	md_addr_t tag_dir = CACHE_TAG (cache_dl2, addr);
	md_addr_t set_dir = CACHE_SET (cache_dl2, addr);
	md_addr_t bofs_dir = CACHE_BLK (cache_dl2, addr);

	int block_offset = blockoffset(addr);
	int isL2Hit = 0;

	/* low-associativity cache, linear search the way list */
	for (blk = cache_dl2->sets[set_dir].way_head; blk; blk = blk->way_next)
	{
		if ((blk->tagid.tag == tag_dir) && (blk->status & CACHE_BLK_VALID))
		{
			isL2Hit = 1;
			if (blk->way_prev && cache_dl2->policy == LRU)
				update_way_list(&cache_dl2->sets[set_dir], blk, Head);
			break;
		}
	}

	if(isL2Hit)	/* Hit in L2 cache, add to sharers list */
	{
		if(strcmp (cp->name, "il1"))
#ifdef NEW_WARMUP
		{
			if(cmd == Read)
			{
				if(blk->dir_blk_state[block_offset] == MESI_INVALID)
				{
					blk->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
				}
				if(blk->dir_blk_state[block_offset] == MESI_MODIFIED || blk->dir_blk_state[block_offset] == MESI_EXCLUSIVE)
				{
					if(blk->dir_sharer[block_offset][threadid/64] & ((unsigned long long int)1<<(threadid%64)) != ((unsigned long long int)1<<(threadid%64)))
						blk->dir_blk_state[block_offset] = MESI_SHARED;
				}
				blk->dir_sharer[block_offset][threadid/64] |= ((unsigned long long int)1<<(threadid%64));
			}	 
			else
			{
				blk->dir_sharer[block_offset][threadid/64] = ((unsigned long long int)1<<(threadid%64));
				blk->dir_dirty[block_offset] = threadid;
				blk->dir_blk_state[block_offset] = MESI_MODIFIED;
			}
		}
		repl1 = blk;
#else
			blk->dir_sharer[block_offset][threadid/64] |= ((unsigned long long int)1<<(threadid%64));
#endif
	}
	else
	{/* L2: Find the replacement, invalidate the old sharers list, and insert the new cache line */
		switch (cache_dl2->policy)
		{
			int i;
			int bindex;
			case LRU:
			case FIFO:
			i = cache_dl2->assoc;
			do {
				if(i == 0)	break;
				i--;
				repl = cache_dl2->sets[set_dir].way_tail;
				update_way_list (&cache_dl2->sets[set_dir], repl, Head);
			}
			while(repl_dir_state_check(repl->dir_state, threadid, repl->addr));

			if(repl_dir_state_check(repl->dir_state, threadid, repl->addr))
				panic("Can not allocate L2 line during warmup");
			break;

			case Random:
			bindex = myrand () & (cache_dl2->assoc - 1);
			repl = CACHE_BINDEX (cache_dl2, cache_dl2->sets[set_dir].blks, bindex);
			break;
			default:
			panic ("bogus replacement policy");
		}

		if (repl->status & CACHE_BLK_VALID)
		{  /* replace the replacement line in L2 cache */

			int i=0, m=0, iteration = 1, flag = 0, sharer_num = 0, Threadid;
			for(i=0; i<(cache_dl2->set_shift - cache_dl1[0]->set_shift); i++)
				iteration = iteration * 2; 
			for (i=0; i<iteration; i++)
				if(Is_Shared(repl->dir_sharer[i], -1))
					sharer_num ++;

			for(i=0; i<iteration; i++)
			{
				repl->dir_state[i] = DIR_STABLE;
				if(Is_Shared(repl->dir_sharer[i], -1))
				{  /* invalidate L1 cache of sharers -  invalidate multiple sharers */
					repl->addr = (repl->tagid.tag << cache_dl2->tag_shift) + (set_dir << cache_dl2->set_shift) + (i << cache_dl1[0]->set_shift);
					for (Threadid = 0; Threadid < numcontexts; Threadid++)
					{
						if(((repl->dir_sharer[i][Threadid/64]) & ((unsigned long long int)1 << (Threadid%64))) == ((unsigned long long int)1 << (Threadid%64)))
						{/* Find the line and invalidate it. */
							md_addr_t temp_tag = CACHE_TAG (cache_dl1[Threadid], addr);
							md_addr_t temp_set = CACHE_SET (cache_dl1[Threadid], addr);
							for (blk = cache_dl1[Threadid]->sets[temp_set].way_head; blk; blk = blk->way_next)
							{
								if ((blk->tagid.tag == temp_tag) && (blk->status & CACHE_BLK_VALID))
								{
									blk->state = MESI_INVALID;
									blk->status = 0;
									break;
								}
							}
						}
					}
					repl->dir_sharer[i][0] = 0;
					repl->dir_sharer[i][1] = 0;
					repl->dir_sharer[i][2] = 0;
					repl->dir_sharer[i][3] = 0;
				}
			}
		}

		/* finish the memory read*/
		repl->dir_state[block_offset] = DIR_STABLE;
		repl->tagid.tag = tag_dir;
		repl->tagid.threadid = threadid;
		repl->addr = addr;
		repl->status = CACHE_BLK_VALID;
		repl->ready = 0;
		repl->dir_dirty[block_offset] = -1;
#ifdef NEW_WARMUP
		if(cmd == Read)
		{
			repl->dir_blk_state[block_offset] = MESI_EXCLUSIVE;
			repl->dir_dirty[block_offset] = -1;
		}
		else
		{
			repl->dir_blk_state[block_offset] = MESI_MODIFIED;
			if(strcmp (cp->name, "il1"))
				repl->dir_dirty[block_offset] = threadid;
			else
				repl->dir_dirty[block_offset] = -1;
		}
#else
		repl->dir_blk_state[block_offset] = MESI_SHARED;
#endif

		if(strcmp (cp->name, "il1"))
			repl->dir_sharer[block_offset][threadid/64] = (unsigned long long int)1<<(threadid%64);
		else
		{
			repl->dir_sharer[block_offset][0] = 0;
			repl->dir_sharer[block_offset][1] = 0;
			repl->dir_sharer[block_offset][2] = 0;
			repl->dir_sharer[block_offset][3] = 0;
		}

		cache_dl2->last_tagsetid.tag = 0;
		cache_dl2->last_tagsetid.threadid = threadid;
		cache_dl2->last_blk = repl;

		if (cache_dl2->hsize)
			link_htab_ent(cache_dl2, &cache_dl2->sets[set_dir], repl);
#ifdef NEW_WARMUP	
		repl1 = repl;
#endif
	}

	/* Allocate an entry in L1 cache */
	switch ((cp)->policy)
	{
		int bindex;
		case LRU:
		case FIFO:
		repl = (cp)->sets[set].way_tail;
		update_way_list (&(cp)->sets[set], repl, Head);
		break;
		case Random:
		bindex = myrand () & ((cp)->assoc - 1);
		repl = CACHE_BINDEX ((cp), (cp)->sets[set].blks, bindex);
		break;
		default:
		panic ("bogus replacement policy");
	}

	repl->tagid.tag = tag;
	repl->tagid.threadid = threadid;
	repl->addr = addr;
	repl->status = CACHE_BLK_VALID;           
#ifdef NEW_WARMUP
	repl->state = repl1->dir_blk_state[block_offset];
#else
	repl->state = MESI_SHARED;
#endif
	repl->ready = 0;

	cp->last_tagsetid.tag = 0;
	cp->last_tagsetid.threadid = threadid;
	cp->last_blk = repl;

	if ((cp)->hsize)
		link_htab_ent((cp), &(cp)->sets[set], repl);
}

int isLineInvalidated(md_addr_t addr, int threadid)
{
	struct cache_t *cp = cache_dl1[threadid];	
	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *blk;
	/* low-associativity cache, linear search the way list */
	for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
	{
		if ((blk->tagid.tag == tag) && !(blk->status & CACHE_BLK_VALID))
			return 1;
	}
	return 0;
}

	void
cache_print (struct cache_t *cp1, struct cache_t *cp2)
{				/* Print cache access stats */
	fprintf (stderr, "ZZ L1D stats\n");
	fprintf (stderr, "ZZ %ld %ld %ld %ld %lu\n", rm1, rh1, wm1, wh1, (unsigned long) cp1->writebacks);
	fprintf (stderr, "ZZ UL2 stats\n");
	if (cp2)
		fprintf (stderr, "ZZ %ld %ld %ld %ld %lu\n", rm2, rh2, wm2, wh2, (unsigned long) cp2->writebacks);
}


	int
simple_cache_flush (struct cache_t *cp)
{
	int i, numflush;
	struct cache_blk_t *blk;

#ifdef TOKENB
	fatal ("need to update this func to account for TOKENB coherence.");
#endif

	numflush = 0;
	for (i = 0; i < cp->nsets; i++)
	{
		for (blk = cp->sets[i].way_head; blk; blk = blk->way_next)
		{
			if (blk->status & CACHE_BLK_VALID)
			{
				//cp->invalidations++;
				blk->status = 0;	//&= ~CACHE_BLK_VALID;
				blk->isStale = 0;

				if (blk->status & CACHE_BLK_DIRTY)
				{
					if (collect_stats)
						cp->writebacks++;
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
				//if (collect_stats)
				//cp->invalidations++;
				blk->status = 0;	//&= ~CACHE_BLK_VALID;
				blk->isStale = 0;
				blk->state = MESI_INVALID;
				blk->invCause = 0;
			}
		}
	}
	return lat;
}

/* Copy nbytes of data from/to cache block */
	void
cacheBcopy (enum mem_cmd cmd, struct cache_blk_t *blk, int position, byte_t * data, int nbytes)
{
	CACHE_BCOPY (cmd, blk, position, data, nbytes);
}

int isCacheHit (struct cache_t *cp, md_addr_t addr, int threadid)
{
	if (!strcmp (cp->name, "dl1") && cp->threadid != threadid)
		panic ("Wrond DL1 cache being called in cache access\n");

	md_addr_t tag = CACHE_TAG (cp, addr);
	md_addr_t set = CACHE_SET (cp, addr);
	md_addr_t bofs = CACHE_BLK (cp, addr);
	struct cache_blk_t *blk;

	if (cp->hsize)
	{
		/* higly-associativity cache, access through the per-set hash tables */
		int hindex = (int) CACHE_HASH (cp, tag);

		for (blk = cp->sets[set].hash[hindex]; blk; blk = blk->hash_next)
		{
			if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
				return 1;
		}
	}
	else
	{
		/* low-associativity cache, linear search the way list */
		for (blk = cp->sets[set].way_head; blk; blk = blk->way_next)
		{
			if ((blk->tagid.tag == tag) && (blk->status & CACHE_BLK_VALID))
				return 1;
		}
	}
	return 0;
}

#if 	defined(BUS_INTERCONNECT) || defined(CROSSBAR_INTERCONNECT)
	void
busInit ()
{
	int i, j;

	j = BUSSLOTS * NUMBEROFBUSES;

	struct bs_nd *link;

	busNodePool = NULL;

	for (i = 0; i < j; i++)
	{
		link = calloc (1, sizeof (struct bs_nd));
		if (!link)
			fatal ("out of virtual memory");
		link->next = busNodePool;
		busNodePool = link;
	}
#ifdef BUS_INTERCONNECT
	for (i = 0; i < NUMBEROFBUSES; i++)
		busNodesInUse[i] = NULL;
#endif
}
#endif



	int
L2inclusionFunc (struct cache_t *cp, md_addr_t addr)
{
	int i, cnt, j;
	int bsize = cp->bsize;
	int mask = ~cp->blk_mask;
	md_addr_t tAddr;
	struct cache_t *cp2;
	md_addr_t tag;
	md_addr_t set;
	struct cache_blk_t *blk;

	cnt = (cp->bsize / cache_dl1[0]->bsize);
	tAddr = (addr & mask);
	for (i = 0; i < numcontexts; i++)
	{
#ifdef	EDA
		if (thecontexts[i]->masterid != 0)
			continue;
#endif
		cp2 = cache_dl1[i];
		for (j = 0; j < cnt; j++)
		{
			addr = tAddr + (j * cp2->bsize);
			tag = CACHE_TAG (cp2, addr);
			set = CACHE_SET (cp2, addr);

			for (blk = cp2->sets[set].way_head; blk; blk = blk->way_next)
			{
#ifdef SMT_SS
				if ((blk->tagid.tag == tag) && (blk->tagid.threadid == i) && (blk->status & CACHE_BLK_VALID))
#else
					if (blk->tag == tag && (blk->status & CACHE_BLK_VALID))
#endif
						break;
			}
			if (blk)
			{
				blk->status = 0;	//&= ~CACHE_BLK_VALID;
				blk->state = MESI_INVALID;
				blk->invCause = 0;
				update_way_list (&cp2->sets[set], blk, Tail);
			}
		}
	}
	/* TODO: We also need to simulate bus delay here */
	return cache_dl1[0]->hit_latency;
}


