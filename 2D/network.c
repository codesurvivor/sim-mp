/* 
 	NETWORK CONFIGURATION FILE
	In this c file, the network access is defined. 

	demenstration for network: network id distribution
	MC is the memory controller location. Empth[] stead for a unuse node, we can also put MC there. Those nodes are configurable. 
	Left numbers in the [] stead for threadid or core ID. Right numbers in the [] stread for the ID for network mapping. 

	 *************************************************************
	column
line-->  [  ]==[MC]==[  ]==[MC]			[00]==[01]==[02]==[03]
	  ||    ||    ||    ||		 	 ||    ||    ||    || 
	 [00]==[01]==[02]==[03]			[10]==[11]==[12]==[13]
	  ||    ||    ||    ||			 ||    ||    ||    || 
	 [04]==[05]==[06]==[07]			[20]==[21]==[22]==[23]
	  ||    ||    ||    ||	   -------->	 ||    ||    ||    || 
	 [08]==[09]==[10]==[11]			[30]==[31]==[32]==[33]
	  ||    ||    ||    ||		  	 ||    ||    ||    ||   
	 [12]==[13]==[14]==[15]		 	[40]==[41]==[42]==[43]
	  ||    ||    ||    ||		  	 ||    ||    ||    || 
	 [  ]==[MC]==[  ]==[MC]		  	[50]==[51]==[52]==[53]

	 <-----Mesh size ----->			  Network ID mapping


	 *************************************************************
	Combinning Network 
		    |
	 [  ]==[MC] | [  ]==[MC]			  |	
	  ||    ||  |  ||    ||		      Chip0 ID(00)| Chip1 ID(01) 
	 [00]==[01] | [02]==[03]		FSOI/MESH | FSOI/MESH   <---Network configuration within a chip	
	  ||    ||  |  ||    ||				  |	
	 [04]==[05] | [06]==[07]		 	  |	
	------------+------------  ----------> -------MESH/FSOI-------  <---Network configuration between chip
	 [08]==[09] | [10]==[11]			  |	
	  ||    ||  |  ||    ||		      Chip2 Id(10)| Chip3 ID(11) 
	 [12]==[13] | [14]==[15]		FSOI/MESH | FSOI/MESH 
	  ||    ||  |  ||    ||		 		  | 
	 [  ]==[MC] | [  ]==[MC]		 	  | 
		    |

	 *************************************************************
 */
#include "headers.h"

#include "smt.h"
#ifdef SMT_SS
#include "context.h"
#endif

extern counter_t popnetMsgNo;
extern counter_t Input_queue_full;
extern counter_t last_Input_queue_full[MAXTHREADS+MESH_SIZE*2];
extern struct cache_t *cache_dl1[], *cache_dl2;
extern counter_t MetaPackets_1;
extern counter_t DataPackets_1;
extern counter_t TotalMetaPacketsInAll, TotalDataPacketsInAll, TotalSyncMetaPackets, TotalSyncDataPackets;
extern counter_t optical_data_packets, optical_meta_packets, mesh_data_packets, mesh_meta_packets;
extern counter_t totalOptHopCount, totalOptHopDelay, totalOptNorHopCount, totalOptNorHopDelay, totalOptSyncHopCount, totalOptSyncHopDelay;
extern counter_t totalMeshHopCount, totalMeshHopDelay, totalEventCount, totalEventProcessTime;
extern counter_t totalMeshNorHopCount, totalMeshNorHopDelay, totalMeshSyncHopCount, totalMeshSyncHopDelay;
extern counter_t totalUpgrades;
extern counter_t totalNetUpgrades;
extern counter_t totalUpgradesUsable;
extern counter_t totalUpgradesBeneficial;
extern counter_t data_packets_sending[300][300];
extern counter_t meta_packets_sending[300][300];
counter_t  meta_packet_dir[300][2];
extern counter_t meta_even_req_close[300];
extern counter_t meta_odd_req_close[300];
extern counter_t req_spand[261];

extern counter_t link_ser_lat[300][300]; /* FIXME: if threadnum is more than 64, large array has to be clarified instread 100x100*/

extern counter_t totalNetWrites;
extern counter_t totalWritesUsable;
extern counter_t totalWritesBeneficial;
extern counter_t exclusive_somewhere;
extern counter_t front_write;
extern lock_network_access;
void IDChangeForIntraNetwork(int src1, int src2, int des1, int des2, int *new_src1, int *new_src2, int *new_des1, int *new_des2);
#ifdef EUP_NETWORK
extern counter_t EUP_entry_full;
#endif
extern short m_shSQSize;		//Total number of entries in Store Queue.
extern int collect_stats;


void network_reg_options(struct opt_odb_t *odb)
{ /* configuration input */
	opt_reg_string (odb, "-network", "network type {FSOI|MESH|COMB|HYBRID}", &network_type, /* default */ "FSOI",
			/* print */ TRUE, /* format */ NULL);

	/* if network type is COMB, then the network configuration should be defined both in inter-chip and intra-chip */
	opt_reg_string (odb, "-network:intercfg", "inter chip network config {FSOI|MESH}", &inter_cfg, "FSOI", /* print */ TRUE, NULL);
	opt_reg_string (odb, "-network:intracfg", "intra chip network config {FSOI|MESH}", &intra_cfg, "MESH", /* print */ TRUE, NULL);

	/* if total number of chips larger than 1, it means the the multi chip simulation */ 
	opt_reg_int (odb, "-network:chipnum", "number of chips in whole system", &total_chip_num, /* default */ 4,
			/* print */ TRUE, /* format */ NULL);	
	optical_options(odb);
	popnet_options(odb);
}

void reply_statistics(counter_t req_time)
{
	if(sim_cycle - req_time < 100)
		req_spand[sim_cycle-req_time] ++;
	else if(sim_cycle - req_time >=100 && sim_cycle - req_time < 120)
		req_spand[100]++;
	else if(sim_cycle - req_time >=120 && sim_cycle - req_time < 140)
		req_spand[101]++;
	else if(sim_cycle - req_time >=140 && sim_cycle - req_time < 160)
		req_spand[102]++;
	else if(sim_cycle - req_time >=160 && sim_cycle - req_time < 180)
		req_spand[103]++;
	else if(sim_cycle - req_time >=180 && sim_cycle - req_time < 200)
		req_spand[104]++;
	else if(sim_cycle - req_time >= 200 && sim_cycle - req_time < 350)
		req_spand[sim_cycle-req_time-95] ++;
	else if(sim_cycle - req_time >= 350 && sim_cycle - req_time < 400)
		req_spand[255] ++;
	else if(sim_cycle - req_time >= 400 && sim_cycle - req_time < 450)
		req_spand[256] ++;
	else if(sim_cycle - req_time >= 450 && sim_cycle - req_time < 550)
		req_spand[257] ++;
	else if(sim_cycle - req_time >= 550 && sim_cycle - req_time < 650)
		req_spand[258] ++;
	else if(sim_cycle - req_time >= 650 && sim_cycle - req_time < 850)
		req_spand[259] ++;
	else if(sim_cycle - req_time >= 850)
		req_spand[260] ++;
}
void network_check_options()
{
	if(!mystricmp (network_type, "COMB"))
	{
		if(total_chip_num == 0)
			fatal("Single chip can not use hybrid network system!\n");
		if(total_chip_num > MAXTHREADS)
			fatal("number of chip exceed the number of threads!\n");
		if(MAXTHREADS == 16)
		{
			if(!(total_chip_num == 2 || total_chip_num == 4 || total_chip_num == 8))
				fatal("number of chip exceed the number of threads!\n");
		}
		if(MAXTHREADS == 32)
		{
			/* FIXME: hard code for 16 and 64, which are rectangular, and no mesh size for 32 */
			fatal("number of chip exceed the number of threads!\n");
		}
		if(MAXTHREADS == 64)
		{
			if(!(total_chip_num == 2 || total_chip_num == 4 || total_chip_num == 8 || total_chip_num == 16 || total_chip_num == 32))
				fatal("number of chip exceed the number of threads!\n");
		}
	}
	Line_chip_num = 2;
	Column_chip_num = total_chip_num/Line_chip_num;
	return;
}


int ChipIdCheck(int a1, int a2)
{
	if(total_chip_num == 0)
		fatal("Single chip can not use hybrid network system!\n");

	int line_size = mesh_size;
	int column_size = mesh_size + 2; /* Array size plus two lines for memory controller */
		
	if(total_chip_num != 4)
		panic("only hard code for chip number is 4\n");	

	if(a1 < column_size/Column_chip_num)
	{
		if(a2 < line_size/Line_chip_num)
			return 0;	/* belongs to the chip 0 */	
		else
			return 1	/* belongs to the chip 1 */;
	}
	else
	{
		if(a2 < line_size/Line_chip_num)
			return 2;	/* belongs to the chip 2 */	
		else
			return 3	/* belongs to the chip 3 */;
	}
}

 /* why I change the ID for inter network access? because if you want to perform inter network access and intra network access both MESH or FSOI, 
     basically you have to use a secondary level network forum to perfrom the communication between inter chip. This ID change serves as the secondary
     level network forum, and use the two more extra lines to do this communications */
void IDChangeForIntraNetwork(int src1, int src2, int des1, int des2, int *new_src1, int *new_src2, int *new_des1, int *new_des2)
{
	int ChipID_x = ChipIdCheck(src1, src2);
	int ChipID_y = ChipIdCheck(des1, des2);
	int x_start = 0, y_start = mesh_size;
	*new_src1 = x_start + ChipID_x/2;
	*new_src2 = y_start + ChipID_x%2;
	*new_des1 = x_start + ChipID_y/2;
	*new_des2 = y_start + ChipID_y%2;
	return;
}
/* check the communication belongs to which network configuration */
int ConfigCheck(int src1, int src2, int des1, int des2)
{
	int src_chip = ChipIdCheck(src1, src2);
	int des_chip = ChipIdCheck(des1, des2);	
	if(src_chip == des_chip)
	{
		/* belongs to the same chip */
		if(!mystricmp(intra_cfg, "FSOI"))
			return FSOI_NET_INTRA;  /* FSOI network configuration within chip */  
		else 	
			return MESH_NET_INTRA;   /* MESH network configuration within chip */
	}
	else
	{
		/* belongs to different chips */
		if(!mystricmp(inter_cfg, "FSOI"))
			return FSOI_NET_INTER;  /* FSOI network configuration between chips */  
		else 	
			return MESH_NET_INTER;  /* MESH network configuration between chips */ 
	}
}
int CombNetworkBufferSpace(int src1, int src2, int des1, int des2, md_addr_t addr, int operation, int *vc)
{
	int options = -1, buffer_full = 0;
	int new_src1, new_src2, new_des1, new_des2;
	IDChangeForIntraNetwork(src1, src2, des1, des2, &new_src1, &new_src2, &new_des1, &new_des2);
	int flag = ConfigCheck(src1, src2, des1, des2);

	if(flag == 0 || flag == 2)
	{ /* belongs to FSOI network */
		if(flag == 2)
		{ /* Intra chip */
			if(opticalBufferSpace(new_src1, new_src2, operation))
				buffer_full = 1;
		}
		else
		{ /* Inter chip */
			if(opticalBufferSpace(src1, src2, operation))
				buffer_full = 1;
		}
	}
	else
	{ /* belongs to MESH network */
		if(flag == 3)
		{ /* Intra chip */
			options = OrderCheck(new_src1, new_src2, new_des1, new_des2, addr);
			*vc = popnetBufferSpace(new_src1, new_src2, options);
			if(*vc == -1)
				buffer_full = 1;
		}
		else
		{ /* Inter chip */
			options = OrderCheck(src1, src2, des1, des2, addr);
			*vc = popnetBufferSpace(src1, src2, options);
			if(*vc == -1)
				buffer_full = 1;
		}
	}
	return buffer_full;
}
#ifdef LOCK_HARD
void schedule_network(struct LOCK_EVENT *event)
{
	int flag = local_access_check(event->src1, event->src2, event->des1, event->des2);
	int type = 0;
	if(event->opt == ACQ_REMOTE_ACC || event->opt == REL_REMOTE_ACC)
		type = meta_packet_size;	
	popnetMsgNo++;
	if(!flag && collect_stats)
	{
		event->when = sim_cycle + 1;
		lock_network_access ++;
		/* insert into network if it's not local access */
		if (!mystricmp (network_type, "FSOI"))
			directMsgInsert(event->src1, event->src2, event->des1, event->des2, sim_cycle, meta_packet_size, popnetMsgNo, 0, event->opt, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,0);
		else if(!mystricmp (network_type, "MESH"))
			popnetMessageInsert(event->src1, event->src2, event->des1, event->des2, sim_cycle, meta_packet_size, popnetMsgNo, 0, event->opt, 0);
		event->when = sim_cycle + WAIT_TIME;
		
	}
	else
		event->when = sim_cycle;
	event->popnetMsgNo = popnetMsgNo;
}
int LockMsgComplete(long w, long x, long y, long z, long long int msgNo)
{
	struct LOCK_EVENT *event, *prev;
	event = lock_event_queue;
	prev = NULL;
	if (event!=NULL)
	{
		while (event)
		{
			if(event->src1 == w && event->src2 == x && event->des1 == y && event->des2 == z && msgNo == event->popnetMsgNo)
			{
				event->when = sim_cycle;
				return 1;
			}
			prev = event;
			event = event->next;
		}
	}
	panic("Error finding event for the returned message from network");
}
#endif

/* Schedule an event through network */
void scheduleThroughNetwork(struct DIRECTORY_EVENT *event, long start, unsigned int type, int vc)
{
	if(event->src1 < 0 || event->src1 > 16 || event->src2 < 0 || event->src2 > 16 || event->des1 < 0 || event->des1 > 16 || event->des2 < 0 || event->des2 > 16)
		panic("Network: source or destination addresses are wrong.");

	int flag = local_access_check(event->src1, event->src2, event->des1, event->des2);
	int even_flag = 0;
	popnetMsgNo++;
	/* for load link optimization, it's a HACK, should be removed */
	if(event->rs)
		if(event->rs->op == LL)
			even_flag = 1;

	/* statistics collecting for sync instructions */
	if(event->isSyncAccess)
	{
		if(type == data_packet_size)
			data_packets_sending[event->src1*(mesh_size+2)+event->src2][event->des1*(mesh_size+2)+event->des2] ++;
		else
		{
			meta_packets_sending[event->src1*(mesh_size+2)+event->src2][event->des1*(mesh_size+2)+event->des2] ++;
			if((event->des1*(mesh_size+2)+event->des2) % 2== 0)
			{/* sending out the request to even nodes */
				if(sim_cycle - meta_packet_dir[event->src1*(mesh_size+2)+event->src2][0] <10)
					meta_even_req_close[event->src1*(mesh_size+2)+event->src2] ++;
				meta_packet_dir[event->src1*(mesh_size+2)+event->src2][0] = sim_cycle;
			}
			else
			{
				if(sim_cycle - meta_packet_dir[event->src1*(mesh_size+2)+event->src2][1] <10)
					meta_odd_req_close[event->src1*(mesh_size+2)+event->src2] ++;
				meta_packet_dir[event->src1*(mesh_size+2)+event->src2][1] = sim_cycle;
			}
		}
	}

	if (!mystricmp (network_type, "FSOI"))
	{ /* optical network schedule */
		if(!flag)
		{ /* for non-local access, go the network */
			if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
			{ /* before go to the network, first check the outgoing buffer in network is full or not for those first class requests */
				if(opticalBufferSpace(event->src1, event->src2, event->operation))
				{
					event->input_buffer_full = 1;
					event->when = start+RETRY_TIME; /*retry again next cycle */		
					if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
					{
						Input_queue_full ++;
						last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
					}
					return 0;
				}	
			}
			/* insert into network if it's not local access */
			directMsgInsert(event->src1, event->src2, event->des1, event->des2, start, type, popnetMsgNo, event->addr, event->operation, event->delay, event->L2miss_flag, event->prefetch_next, event->dirty_flag, event->arrival_time, event->data_reply, event->conf_bit, 0, even_flag, event->isSyncAccess, event->store_cond
#ifdef EXCLUSIVE_OPT
			, event->exclusive_after_wb
#endif
			);
			event->when = start + WAIT_TIME; /* after comming back from the network, will wake up this event at the directory fifo queue or l1 fifo queue */
		}
		else
			event->when = start; /* for local access */
		event->popnetMsgNo = popnetMsgNo;
	}
	else if(!mystricmp (network_type, "MESH"))
	{ /* mesh network schedule */
		if(!flag)
		{ /*non-local accesses, go the network */
			int options = -1;
			options = OrderCheck(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1));
			vc = popnetBufferSpace(event->src1, event->src2, options);
			if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
			{ /* for this first class request, check the outgoing buffer in network */
				if(vc == -1)
				{ /* vc is -1 means the outgoing buffer is full */
					event->input_buffer_full = 1;
					event->when = start+RETRY_TIME;	/* retry again */	
					if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
					{
						Input_queue_full ++;
						last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
					}
					return 0;
				}	
			}

			if(vc == -1)
			{ /* vc is -1 means the outgoing buffer is full, overflow in outgoing buffer  */ 
				if(options == -1)
					vc = 0; // FIXME: hard core here, 1 should be the number of virtual channel 
				else
					vc = options;
			}
			if(type == data_packet_size)
				DataPackets_1 ++;
			else
				MetaPackets_1 ++;
	#ifndef MULTI_VC
			vc = 0;
	#endif
			//printf("wxh debug Msg No: %d, op %d", popnetMsgNo,event->operation);
			//printf(" cache %s", event->cp->name);
			//if(event->rs!= NULL)
			//{
			// printf (" rs PC %x",event->rs->PC);
			//}
			//printf("\n");
			fflush(stdout);
			popnetMessageInsert(event->src1, event->src2, event->des1, event->des2, start, type, popnetMsgNo, event->addr>>cache_dl2->set_shift, event->operation, vc);

	#ifdef MULTI_VC
			/*Insert the request into table to record which vc it used */
			OrderBufInsert(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1), vc, popnetMsgNo);
	#endif
			event->when = start + WAIT_TIME;
		}
		else
			event->when = start;
		event->popnetMsgNo = popnetMsgNo;
	}
	else if (!mystricmp (network_type, "HYBRID"))
	{ /* On-chip hybrid network insert */
		if((abs(event->des1 - event->src1) + abs(event->des2 - event->src2)) > 1)
		{ /* larger distance using optical network */
			/* optical network schedule */
			if(!flag)
			{
				if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
				{
					if(opticalBufferSpace(event->src1, event->src2, event->operation))
					{
						event->input_buffer_full = 1;
						event->when = start+RETRY_TIME;		
						if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
						{
							Input_queue_full ++;
							last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
						}
						return 0;
					}	
				}
				/* insert into network if it's not local access */
				directMsgInsert(event->src1, event->src2, event->des1, event->des2, start, type, popnetMsgNo, event->addr, event->operation, event->delay, event->L2miss_flag, event->prefetch_next, event->dirty_flag, event->arrival_time, event->data_reply, event->conf_bit, 0, even_flag, event->isSyncAccess, event->store_cond
#ifdef EXCLUSIVE_OPT
			, event->exclusive_after_wb
#endif
				);

				if(type == data_packet_size)
					optical_data_packets ++;
				else
					optical_meta_packets ++;
				event->when = start + WAIT_TIME;
			}
			else
				event->when = start;
			event->popnetMsgNo = popnetMsgNo;
		}
		else
		{
			/* electrical link network schedule */
			if(!flag)
			{
				if(type == data_packet_size)
					link_ser_lat[event->src1*mesh_size+event->src2][event->des1*mesh_size+event->des2] += type; /* serilaize latency */
				else 
					link_ser_lat[event->src1*mesh_size+event->src2][event->des1*mesh_size+event->des2] += 1; /* serilaize latency */

				event->when = start + link_ser_lat[event->src1*mesh_size+event->src2][event->des1*mesh_size+event->des2];
				if(type == data_packet_size)
					mesh_data_packets ++;
				else
					mesh_meta_packets ++;
				
			}
			else
				event->when = start;
			event->popnetMsgNo = popnetMsgNo;
		}
	}
	else
	{ /* Multi-chip hybrid network insert */
		if(!flag)
		{
			int Chip_identify = ConfigCheck(event->src1, event->src2, event->des1, event->des2);
			if(Chip_identify == FSOI_NET_INTER || Chip_identify == FSOI_NET_INTRA)
			{
				/* Belongs to the FSOI network*/
				if(Chip_identify == FSOI_NET_INTER)
				{
					/* Chip_identify == FSOI_NET_INTER means the inter chip network configuration is FSOI, use secondary level network forum to transfer the data */
					int new_src1, new_src2, new_des1, new_des2;
					IDChangeForIntraNetwork(event->src1, event->src2, event->des1, event->des2, &new_src1, &new_src2, &new_des1, &new_des2);
					if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
					{
						if(opticalBufferSpace(new_src1, new_src2, event->operation))
						{
							event->input_buffer_full = 1;
							event->when = start+1;		
							if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
							{
								Input_queue_full ++;
								last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
							}
							return 0;
						}	
					}
					/* insert into network if it's not local access */
					directMsgInsert(new_src1, new_src2, new_des1, new_des2, start, type, popnetMsgNo, event->addr, event->operation, event->delay, event->L2miss_flag, event->prefetch_next, event->dirty_flag, event->arrival_time, event->data_reply, event->conf_bit, 0, even_flag, event->isSyncAccess, event->store_cond
#ifdef EXCLUSIVE_OPT
			, event->exclusive_after_wb
#endif
					);
					event->when = start + WAIT_TIME;
					event->new_src1 = new_src1;
					event->new_src2 = new_src2;
					event->new_des1 = new_des1;
					event->new_des2 = new_des2;
					
				}
				else  
				{
					/* Chip_identify == FSOI_NET_INTRA means the intra chip network configruation is FSOI */
					if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
					{
						if(opticalBufferSpace(event->src1, event->src2, event->operation))
						{
							event->input_buffer_full = 1;
							event->when = start+1;		
							if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
							{
								Input_queue_full ++;
								last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
							}
							return 0;
						}	
					}
					/* insert into network if it's not local access */
					directMsgInsert(event->src1, event->src2, event->des1, event->des2, start, type, popnetMsgNo, event->addr, event->operation, event->delay, event->L2miss_flag, event->prefetch_next, event->dirty_flag, event->arrival_time, event->data_reply, event->conf_bit, 0, even_flag, event->isSyncAccess, event->store_cond
#ifdef EXCLUSIVE_OPT
			, event->exclusive_after_wb
#endif
);
					event->when = start + WAIT_TIME;
				}
			}
			else 
			{
				/* Belongs to the MESH network*/
				int options = -1;
				if(Chip_identify == MESH_NET_INTER)
				{
					/* Chip_identify == MESH_NET_INTER means the inter chip network configuration is MESH */
					int new_src1, new_src2, new_des1, new_des2;
					IDChangeForIntraNetwork(event->src1, event->src2, event->des1, event->des2, &new_src1, &new_src2, &new_des1, &new_des2);

					options = OrderCheck(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1));
					vc = popnetBufferSpace(event->src1, event->src2, options);

					if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
					{
						if(vc == -1)
						{
							event->input_buffer_full = 1;
							event->when = start+1;		
							if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
							{
								Input_queue_full ++;
								last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
							}
							return 0;
						}	
					}

					if(vc == -1)
					{
						if(options == -1)
							vc = 0; // FIXME: hard core here, 1 should be the number of virtual channel 
						else
							vc = options;
					}
					if(type == data_packet_size)
						DataPackets_1 ++;
					else
						MetaPackets_1 ++;
#ifndef MULTI_VC
					vc = 0;
#endif
					popnetMessageInsert(new_src1, new_src2, new_des1, new_des2, start, type, popnetMsgNo, event->addr>>cache_dl2->set_shift, event->operation, vc);
#ifdef MULTI_VC
					OrderBufInsert(new_src1, new_src2, new_des1, new_des2, event->addr&~(cache_dl1[0]->bsize-1), vc, popnetMsgNo);
#endif
					event->when = start + WAIT_TIME;
					event->new_src1 = new_src1;
					event->new_src2 = new_src2;
					event->new_des1 = new_des1;
					event->new_des2 = new_des2;
					
				}
				else
				{
					/* Chip_identify == MESH_NET_INTRA means the intra chip network configuration is MESH */
					options = OrderCheck(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1));
					vc = popnetBufferSpace(event->src1, event->src2, options);
					if(event->operation == MISS_IL1 || event->operation == MISS_READ || event->operation == MISS_WRITE || event->operation == WRITE_UPDATE)
					{
						options = OrderCheck(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1));
						vc = popnetBufferSpace(event->src1, event->src2, options);
						if(vc == -1)
						{
							event->input_buffer_full = 1;
							event->when = start+1;		
							if(!last_Input_queue_full[event->des1*mesh_size+event->des2])
							{
								Input_queue_full ++;
								last_Input_queue_full[event->des1*mesh_size+event->des2] = sim_cycle;
							}
							return 0;
						}	
					}

					if(vc == -1)
					{
						if(options == -1)
							vc = 0; // FIXME: hard core here, 1 should be the number of virtual channel 
						else
							vc = options;
					}
					if(type == data_packet_size)
						DataPackets_1 ++;
					else
						MetaPackets_1 ++;
#ifndef MULTI_VC
					vc = 0;
#endif
					popnetMessageInsert(event->src1, event->src2, event->des1, event->des2, start, type, popnetMsgNo, event->addr>>cache_dl2->set_shift, event->operation, vc);
#ifdef MULTI_VC
					OrderBufInsert(event->src1, event->src2, event->des1, event->des2, event->addr&~(cache_dl1[0]->bsize-1), vc, popnetMsgNo);
#endif
					event->when = start + WAIT_TIME;
				}
			}

		}
		else
			event->when = start;
		event->popnetMsgNo = popnetMsgNo;
	}

	/*collected all the statistics for total number of packets*/
	if(type == data_packet_size)
	{
		TotalDataPacketsInAll ++;
		if(event->isSyncAccess)
			TotalSyncDataPackets ++;
	}
	else
	{
		TotalMetaPacketsInAll ++;
		if(event->isSyncAccess)
			TotalSyncMetaPackets ++;
	}
}



int MsgComplete(long w, long x, long y, long z, counter_t stTime, long long int msgNo, counter_t transfer_time, int delay, counter_t req_time)
{
    /* optical network event complete */
	struct DIRECTORY_EVENT *event, *prev;
	event = dir_event_queue;
	prev = NULL;
	if (event!=NULL)
	{
		while (event)
		{
			int equal_flag = 0;
			if(!mystricmp(network_type, "COMB"))
			{
				int Chip_identify = ConfigCheck(event->src1, event->src2, event->des1, event->des2);
				if(Chip_identify == FSOI_NET_INTER)
				{
					if(event->new_src1 == w && event->new_src2 == x && event->new_des1 == y && event->new_des2 == z && event->startCycle == stTime && !event->processingDone && msgNo == event->popnetMsgNo)
						equal_flag = 1;
				}
				else
				{
					if(event->src1 == w && event->src2 == x && event->des1 == y && event->des2 == z && event->startCycle == stTime && !event->processingDone && msgNo == event->popnetMsgNo)
						equal_flag = 1;
				}
			}
			else
			{
				if(event->src1 == w && event->src2 == x && event->des1 == y && event->des2 == z && event->startCycle == stTime && !event->processingDone && msgNo == event->popnetMsgNo)
					equal_flag = 1;
			}
		
			if(equal_flag)
			{
				struct DIRECTORY_EVENT *temp = event->next;
				event->next = NULL;
				int return_value = dir_fifo_enqueue(event, 0);
				if(return_value == 0)
				{
					event->next = temp;
					return 0;
				}
				else
				{
					/* statistics */
					totalOptHopCount++;  //total optical network accesses 
#ifdef INV_ACK_CON
					if(event->operation == ACK_DIR_WRITEUPDATE)
						totalOptHopDelay += 1; //toal optical network delay for acknowledgement is 1 for INV_ACK_CON optimization 
					else
#endif
						totalOptHopDelay += (sim_cycle - event->startCycle);  //total optical network delay 
					if(event->store_cond)
					{ /* network access and delay count for sync messages */
						totalOptSyncHopCount++;
#ifdef INV_ACK_CON
						if(event->operation == ACK_DIR_WRITEUPDATE)
							totalOptSyncHopDelay += 1;
						else
#endif
							totalOptSyncHopDelay += (sim_cycle - event->startCycle);
					}
					else
					{ /* network access and delay count for normal messages */
						totalOptNorHopCount++;
#ifdef INV_ACK_CON
						if(event->operation == ACK_DIR_WRITEUPDATE)
							totalOptNorHopDelay += 1;
						else
#endif
							totalOptNorHopDelay += (sim_cycle - event->startCycle);
					}
					//if(event->operation == ACK_DIR_WRITE || event->operation == ACK_DIR_READ_SHARED || event->operation == ACK_DIR_READ_EXCLUSIVE || event->operation == ACK_DIR_IL1 || event->operation == ACK_MSG_WRITE || event->operation == ACK_MSG_READ || event->operation == WAIT_MEM_READ || event->operation == WAIT_MEM_READ_N)
					//	reply_statistics(event->req_time);
					event->processingDone = 1;
					event->transfer_time = transfer_time;
					event->delay = delay;
					event->req_time = sim_cycle;
					event->arrival_time = sim_cycle;

					if(prev == NULL)
						dir_event_queue = temp;
					else
						prev->next = temp;
					/* coherence optimization for optical network, try to send early notification for write upgrade and write miss to indicate the write permission through confirmation laser */
					if(!mystricmp(network_type, "FSOI") || (!mystricmp(network_type, "HYBRID")))
					{
#ifdef EUP_NETWORK
						if(event->operation == WRITE_UPDATE || event->operation == MISS_WRITE || event->operation == MISS_READ)
							event->early_flag = 0;
						if(event->operation == WRITE_UPDATE)
						{
							totalNetUpgrades++;
							if(EarlyUpgrade(event))
							{
								if(EUP_entry_check((event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2, event->addr>>cache_dl2->set_shift))
								{
									EarlyUpgrateGenerate(event);
									totalUpgradesUsable++;
									if(thecontexts[(event->src1-MEM_LOC_SHIFT)*mesh_size + event->src2]->m_shSQNum > m_shSQSize-3)
										totalUpgradesBeneficial++;
									EUP_entry_allocate((event->des1-MEM_LOC_SHIFT)*mesh_size + event->des2, event->addr>>cache_dl2->set_shift);	
									event->early_flag = 1;
								}
								else
									EUP_entry_full ++;
							}
						}
#endif
#ifdef WRITE_EARLY
						if(event->operation == MISS_WRITE)
						{
							totalNetWrites ++;
							if(EarlyWrite(event))
							{
								EarlyWriteGenerate(event);
								totalWritesUsable++;
								if(thecontexts[(event->src1-MEM_LOC_SHIFT)*mesh_size + event->src2]->m_shSQNum > m_shSQSize-3)
									totalWritesBeneficial++;
								event->early_flag = 3;
							}
						}
#endif
					}
					return 1;
				}
			}
			prev = event;
			event = event->next;
		}
	}
	panic("Error finding event for the returned message from network");
}
int popnetMsgComplete(long w, long x, long y, long z, counter_t stTime, long long int msgNo)
{
	/* mesh network event complete */
	struct DIRECTORY_EVENT *event, *prev;
	event = dir_event_queue;
	prev = NULL;
	if (event!=NULL)
	{
		while (event)
		{
			int equal_flag = 0;
			if(!mystricmp(network_type, "COMB"))
			{
				int Chip_identify = ConfigCheck(event->src1, event->src2, event->des1, event->des2);
				if(Chip_identify == MESH_NET_INTER)
				{
					if(event->new_src1 == w && event->new_src2 == x && event->new_des1 == y && event->new_des2 == z && event->startCycle == stTime && !event->processingDone && msgNo == event->popnetMsgNo)
						equal_flag = 1;
				}
				else
				{
					if(event->src1 == w && event->src2 == x && event->des1 == y && event->des2 == z && event->startCycle == stTime && !event->processingDone && msgNo == event->popnetMsgNo)
						equal_flag = 1;
				}
			}
			else
			{
				if(event->src1 == w && event->src2 == x && event->des1 == y && event->des2 == z && event->startCycle == stTime && !event->processingDone && msgNo == event->popnetMsgNo)
					equal_flag = 1;
			}
		
			if(equal_flag)
			{
				struct DIRECTORY_EVENT *temp = event->next;
				if(dir_fifo_enqueue(event, 0) == 0)
					return 0;
				else
				{
#ifdef MULTI_VC
					OrderBufRemove(w, x, y, z, event->addr&~(cache_dl1[0]->bsize-1), msgNo);
#endif
					totalMeshHopCount++;
					totalMeshHopDelay += (sim_cycle - event->startCycle);
					if(event->store_cond)
					{
						totalMeshSyncHopCount++;
						totalMeshSyncHopDelay += (sim_cycle - event->startCycle);
					}
					else
					{
						totalMeshNorHopCount++;
						totalMeshNorHopDelay += (sim_cycle - event->startCycle);
					}
					event->processingDone = 1;
					if(prev == NULL)
						dir_event_queue = temp;
					else
						prev->next = temp;
					if(event->operation == WRITE_UPDATE)
						totalNetUpgrades++;
					return 1;
				}
			}
			prev = event;
			event = event->next;
		}
	}
#ifdef LOCK_HARD
	if(!LockMsgComplete(w, x, y, z, msgNo))
		panic("Error finding event for the returned message from network");
#endif
}
#define CACHE_HASH(cp, key)						\
	(((key >> 24) ^ (key >> 16) ^ (key >> 8) ^ key) & ((cp)->hsize-1))

#ifdef WRITE_EARLY
int EarlyWrite(struct DIRECTORY_EVENT *event)
{
	struct cache_blk_t *blk;	//block in cache L2 if there is a L2 hit
	struct cache_t *cp_dir = cache_dl2;//cp1 is the owner L1 cache
	md_addr_t tag_dir, set_dir;
	int hindex_dir;
	md_addr_t addr=event->addr;
	tag_dir = CACHE_TAG(cp_dir, addr);	//L2 cache block tag
	set_dir = CACHE_SET(cp_dir, addr);	//L2 cache block set
	hindex_dir = (int) CACHE_HASH(cp_dir,tag_dir);
	int early_flag1 = 0, early_flag2 = 0, Threadid;
	int block_offset = blockoffset(event->addr);
	int tempID =  event->tempID;	//requester thread id
	int i = 0, des = event->des1*mesh_size+event->des2;
	if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
	{ /* check the block state */
		//if(blk->dir_state[block_offset] == DIR_STABLE)
		{
			if((IsExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid)))// || (blk->pendingInvAck[block_offset] == 0))
			{
				early_flag1 = 1;
				exclusive_somewhere ++;
			}
			else
				early_flag1 = 0;
		}
		//else
		//	early_flag1 = 1;
	}
	//else
	//	early_flag1 = 1;

	if(dir_fifo_num[des])
	{ /* check the dir fifo queue */
		for(i=0;i<(dir_fifo_num[des]);i++)
		{
			if((dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->operation == WRITE_UPDATE || dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->operation == MISS_WRITE || dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->operation == WRITE_BACK) && ((dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->addr>>cache_dl1[0]->set_shift) == (event->addr>>cache_dl1[0]->set_shift)) && dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->popnetMsgNo != event->popnetMsgNo)
			{
				early_flag2 = 1;
				front_write ++;
			}
		}
	}
	if(early_flag2 || early_flag1)
		return 0;
	else 
		return 1;
}
int EarlyWriteGenerate(struct DIRECTORY_EVENT *event)
{
	struct DIRECTORY_EVENT *new_event2 = allocate_event();
	if(new_event2 == NULL)       panic("Out of Virtual Memory");
	new_event2->input_buffer_full = 0;
	new_event2->req_time = 0;
	new_event2->conf_bit = -1;
	new_event2->pending_flag = 0;
	new_event2->op = event->op;
	new_event2->isPrefetch = 0;
	new_event2->operation = ACK_DIR_WRITE_EARLY;
	new_event2->src1 = event->des1;
	new_event2->src2 = event->des2;
	new_event2->des1 = event->src1;
	new_event2->des2 = event->src2;
	new_event2->processingDone = 0;
	new_event2->startCycle = sim_cycle;
	new_event2->parent = event;
	new_event2->tempID = event->tempID;
	new_event2->resend = 0;
	new_event2->blk_dir = NULL;
	new_event2->cp = event->cp;
	new_event2->addr = event->addr;
	new_event2->vp = event->vp;
	new_event2->nbytes = event->cp->bsize;
	new_event2->udata = event->udata;
	new_event2->cmd = event->cmd;
	new_event2->rs = event->rs;
	new_event2->started = event->started;
	new_event2->spec_mode = event->spec_mode;
	new_event2->L2miss_flag = event->L2miss_flag;
	new_event2->data_reply = event->data_reply;
	new_event2->pendingInvAck = 0;
	popnetMsgNo ++;
	new_event2->popnetMsgNo = popnetMsgNo;
	new_event2->when = sim_cycle;
	new_event2->early_flag = 4;

	if(dir_fifo_enqueue(new_event2, 0) == 0)
		dir_eventq_insert(new_event2);
}

#endif
#ifdef EUP_NETWORK
void EUP_entry_init(int id)
{
	int i=0;
	for(i=0;i<MAX_EUP_ENTRY;i++)
	{
		EUP_entry[id].Entry[i].addr = 0;                                                   
		EUP_entry[id].Entry[i].isValid = 0;                                                   
		EUP_entry[id].Entry[i].owner = 0;                                                     
	}
	EUP_entry[id].free_Entries = MAX_EUP_ENTRY;
	return;
}
int EUP_entry_replacecheck(int id, md_addr_t addr)
{
	int i=0, flag=0;
	for(i=0;i<MAX_EUP_ENTRY;i++)
	{
		//if(EUP_entry[id].Entry[i].isValid && ((EUP_entry[id].Entry[i].addr)&cache_dl2->set_mask) == (addr&cache_dl2->set_mask))
		if(EUP_entry[id].Entry[i].isValid && ((EUP_entry[id].Entry[i].addr)) == (addr))
			flag = 1;
	}
	return flag;
}
int EUP_entry_check(int id, md_addr_t addr)
{
	int i=0, flag=0;
	for(i=0;i<MAX_EUP_ENTRY;i++)
	{
		if(EUP_entry[id].Entry[i].isValid && EUP_entry[id].Entry[i].addr == addr)
			flag = 1;
	}
	if(EUP_entry[id].free_Entries == 0 && flag == 0)
		return 0;
	else 
		return 1;
}
void EUP_entry_allocate(int id, md_addr_t addr)
{
	int i = 0;
	for(i=0;i<MAX_EUP_ENTRY;i++)
	{
		if(EUP_entry[id].Entry[i].isValid && EUP_entry[id].Entry[i].addr == addr)
		{
			EUP_entry[id].Entry[i].owner ++;
			return;
		}
	}
	if(EUP_entry[id].free_Entries == 0)
		panic("no free entries for early notification!\n");
	for(i=0;i<MAX_EUP_ENTRY;i++)
	{
		if(!EUP_entry[id].Entry[i].isValid)
		{
			EUP_entry[id].Entry[i].addr = addr;
			EUP_entry[id].Entry[i].isValid = 1;
			EUP_entry[id].Entry[i].owner = 1;
			EUP_entry[id].free_Entries --;
			break;
		}	
	}	
	return;
}

void EUP_entry_dellocate(int id, md_addr_t addr)
{
	int i = 0;
	for(i=0;i<MAX_EUP_ENTRY;i++)
	{
		if(EUP_entry[id].Entry[i].isValid && EUP_entry[id].Entry[i].addr == addr)
		{
			EUP_entry[id].Entry[i].owner --;
			if(EUP_entry[id].Entry[i].owner == 0)
			{
				EUP_entry[id].Entry[i].isValid = 0;	
				EUP_entry[id].Entry[i].addr = 0;	
				EUP_entry[id].free_Entries ++;
			}
			return;
		}
	}
	panic("Can not found the entry for early notification\n");
}
#define CACHE_HASH(cp, key)						\
	(((key >> 24) ^ (key >> 16) ^ (key >> 8) ^ key) & ((cp)->hsize-1))
int EarlyUpgrade(struct DIRECTORY_EVENT *event)
{
	struct cache_blk_t *blk;	//block in cache L2 if there is a L2 hit
	struct cache_t *cp_dir = cache_dl2;//cp1 is the owner L1 cache
	md_addr_t tag_dir, set_dir;
	int hindex_dir;
	md_addr_t addr=event->addr;
	tag_dir = CACHE_TAG(cp_dir, addr);	//L2 cache block tag
	set_dir = CACHE_SET(cp_dir, addr);	//L2 cache block set
	hindex_dir = (int) CACHE_HASH(cp_dir,tag_dir);
	int early_flag1 = 0, early_flag2 = 0, Threadid;
	int block_offset = blockoffset(event->addr);
	int tempID =  event->tempID;	//requester thread id
	int i = 0, des = event->des1*mesh_size+event->des2;
	if(findCacheStatus(cp_dir, set_dir, tag_dir, hindex_dir, &blk))
	{ /* check the block state */
		if(blk->dir_state[block_offset] == DIR_STABLE)
		{
			if(!Is_ExclusiveOrDirty(blk->dir_sharer[block_offset], tempID, &Threadid) && Is_Shared(blk->dir_sharer[block_offset], tempID) && blk->pendingInvAck[block_offset] == 0)
				early_flag1 = 0;
			else
				early_flag1 = 1;
		}
		else
			early_flag1 = 1;
	}
	//else
	//	early_flag1 = 1;

	if(dir_fifo_num[des])
	{ /* check the dir fifo queue */
		for(i=0;i<(dir_fifo_num[des]);i++)
		{
			if((dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->operation == WRITE_UPDATE || (dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->operation == MISS_WRITE && ((dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->addr>>cache_dl1[0]->set_shift) == (event->addr>>cache_dl1[0]->set_shift)))) && dir_fifo[des][(dir_fifo_head[des]+i)%DIR_FIFO_SIZE]->popnetMsgNo != event->popnetMsgNo)
				early_flag2 = 1;
		}
	}
	if(early_flag2 || early_flag1)
		return 0;
	else 
		return 1;
}

int EarlyUpgrateGenerate(struct DIRECTORY_EVENT *event)
{
	struct DIRECTORY_EVENT *new_event2 = allocate_event();
	if(new_event2 == NULL)       panic("Out of Virtual Memory");
	new_event2->input_buffer_full = 0;
	new_event2->req_time = 0;
	new_event2->conf_bit = -1;
	new_event2->pending_flag = 0;
	new_event2->op = event->op;
	new_event2->isPrefetch = 0;
	new_event2->operation = ACK_DIR_WRITEUPDATE;
	new_event2->src1 = event->des1;
	new_event2->src2 = event->des2;
	new_event2->des1 = event->src1;
	new_event2->des2 = event->src2;
	new_event2->processingDone = 0;
	new_event2->startCycle = sim_cycle;
	new_event2->parent = event;
	new_event2->tempID = event->tempID;
	new_event2->resend = 0;
	new_event2->blk_dir = NULL;
	new_event2->cp = event->cp;
	new_event2->addr = event->addr;
	new_event2->vp = event->vp;
	new_event2->nbytes = event->cp->bsize;
	new_event2->udata = event->udata;
	new_event2->cmd = event->cmd;
	new_event2->rs = event->rs;
	new_event2->started = event->started;
	new_event2->spec_mode = event->spec_mode;
	new_event2->L2miss_flag = event->L2miss_flag;
	new_event2->data_reply = event->data_reply;
	new_event2->pendingInvAck = 1;
	popnetMsgNo ++;
	new_event2->popnetMsgNo = popnetMsgNo;
	new_event2->when = sim_cycle;
	new_event2->early_flag = 2;

	if(dir_fifo_enqueue(new_event2, 0) == 0)
		dir_eventq_insert(new_event2);
}
#endif

