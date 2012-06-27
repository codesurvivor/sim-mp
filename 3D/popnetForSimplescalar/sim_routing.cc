#include "sim_router.h"
#include "sim_foundation.h"
#include "mess_queue.h"
#include "mess_event.h"
#include "SRGen.h"
#include "SStd.h"

void sim_router_template::TXY_algorithm(const add_type & des_t,
		const add_type & sor_t, long s_ph, long s_vc)
{
	printf ("don't come here\n");
	assert(0);
}
void sim_router_template::CMESH_XYZ_algorithm(const add_type & des_t,
		const add_type & sor_t, long s_ph, long s_vc)
{
	printf ("don't come here\n");
	assert(0);
}

//***************************************************************************//
void sim_router_template::XYZ_algorithm(const add_type & des_t,
		const add_type & sor_t, long s_ph, long s_vc)
{
	long add_z, add_y, add_x;
	long des_z, des_y, des_x;
	int local_id = address_[0]*ary_size_+address_[1];
	add_type des_network_t ;
	add_type des_t_1 = des_t;
	//FindNetworkID(des_t_1, des_network_t);
	long des_network_ID=des_t[0]*ary_size_+des_t[1];

//long new_add=address_[0]*10+address_[1];
//long new_des=des_t[0]*10+des_t[1];
//add_z=new_add/20;//z
//add_y=new_add%20/5;//y
//add_x=new_add%5;//x
//des_z=new_des/20;//z
//des_y=new_des%20/5;//y
//des_x=new_des%5;//x
	int des_network_z = des_network_ID / ( DIM_X*DIM_Y)  ;
	int des_network_y = (des_network_ID % (DIM_X*DIM_Y) )/ DIM_X;
	int des_network_x = (des_network_ID % (DIM_X*DIM_Y) )% DIM_X;
	
	int local_network_z = local_id / ( DIM_X*DIM_Y)  ;
	int local_network_y = (local_id % (DIM_X*DIM_Y) )/ DIM_X;
	int local_network_x = (local_id % (DIM_X*DIM_Y) )% DIM_X;


	long xoffset = des_network_x - local_network_x;
	long yoffset = des_network_y - local_network_y;
	long zoffset = des_network_z - local_network_z;
	if(zoffset < 0) {
			input_module_.add_routing(s_ph, s_vc, VC_type(5,s_vc));
	}else if(zoffset > 0) {
			input_module_.add_routing(s_ph, s_vc, VC_type(6,s_vc));
	}
	else {
	  if(yoffset < 0) {
			input_module_.add_routing(s_ph, s_vc, VC_type(3,s_vc));
	  }else if(yoffset > 0) {
			input_module_.add_routing(s_ph, s_vc, VC_type(4,s_vc));
	  }else {
	    if(xoffset < 0) {
				input_module_.add_routing(s_ph, s_vc, VC_type(1,s_vc));
	    }else if (xoffset > 0) {
				input_module_.add_routing(s_ph, s_vc, VC_type(2,s_vc));
	    }
	  } 
	}
}
			
//***************************************************************************//
//only two-dimension is supported
void sim_router_template::routing_decision()
{
	time_type event_time = mess_queue::m_pointer().current_time();
	//for injection physical port 0
	for(long j = 0; j < vc_number_; j++) {
		//for the HEADER_ flit
		flit_template flit_t;
		if(input_module_.state(0,j) == ROUTING_) {
			flit_t = input_module_.get_flit(0,j);
			add_type des_t = flit_t.des_addr();
			add_type sor_t = flit_t.sor_addr();
			add_type des_network_addr;
			//FindNetworkID(des_t, des_network_addr);// by wxh
			if(address_ == des_t) {
				if(accept_flit(event_time, flit_t) == 0)
					continue;
				input_module_.remove_flit(0, j);
				input_module_.state_update(0, j, HOME_);
				counter_inc(0, j);
#if 1
				if(flit_t.flit_size() == 1) {
					if(input_module_.input(0, j).size() > 0) {
						input_module_.state_update(0, j, ROUTING_);
					}else {
						input_module_.state_update(0, j, INIT_);
					}
				}
#endif
			}else {
				input_module_.clear_routing(0,j);
				input_module_.clear_crouting(0,j);
				(this->*curr_algorithm)(des_t, sor_t, 0, j);
				input_module_.state_update(0, j, VC_AB_);
			}
		//the BODY_ or TAIL_ flits
		}else if(input_module_.state(0,j) == HOME_)  {
			if(input_module_.input(0, j).size() > 0) {
				flit_t = input_module_.get_flit(0, j);
				Sassert(flit_t.type() != HEADER_);
				if(accept_flit(event_time, flit_t) == 0)
					continue;
				input_module_.remove_flit(0, j);
				counter_inc(0, j);
				if(flit_t.type() == TAIL_) {
					if(input_module_.input(0, j).size() > 0) {
						input_module_.state_update(0, j, ROUTING_);
					}else {
						input_module_.state_update(0, j, INIT_);
					}
				}
			}
		}
	}

	//for other physical ports
	for(long i = 1; i < physic_ports_; i++) {
		for(long j = 0; j < vc_number_; j++) {
			//send back CREDIT message
			flit_template flit_t;


			//for HEADER_ flit
			if(input_module_.state(i, j) == ROUTING_) {
				flit_t = input_module_.get_flit(i, j);
				Sassert(flit_t.type() == HEADER_);
				add_type des_t = flit_t.des_addr();
				add_type sor_t = flit_t.sor_addr();
				add_type des_network_addr;
				//FindNetworkID(des_t, des_network_addr);// by wxh
				if(address_ == des_t) {
					if(accept_flit(event_time, flit_t) == 0)
						continue;
					input_module_.remove_flit(i, j);
					input_module_.state_update(i, j, HOME_);
#if 1
					add_type cre_add_t = address_;
					long cre_pc_t = i;
					ConnectionDownStream( i,  address_, cre_add_t,  cre_pc_t, ary_size_);
					mess_queue::wm_pointer().add_message(
						mess_event(event_time + CREDIT_DELAY_, 
						CREDIT_, address_, cre_add_t, cre_pc_t, j));

					if(flit_t.flit_size() == 1) {
						if(input_module_.input(i, j).size() > 0) {
							input_module_.state_update(i, j, ROUTING_);
						}else {
							input_module_.state_update(i, j, INIT_);
						}
					}
#endif
				}else {
					input_module_.clear_routing(i, j);
					input_module_.clear_crouting(i, j);
					(this->*curr_algorithm)(des_t, sor_t, i, j);
					input_module_.state_update(i, j, VC_AB_);
				}
			//for BODY_ or TAIL_ flits
			}else if(input_module_.state(i, j) == HOME_) {
				if(input_module_.input(i, j).size() > 0) {
					flit_t = input_module_.get_flit(i, j);
					Sassert(flit_t.type() != HEADER_);
					if(accept_flit(event_time, flit_t) == 0)
						continue;
					input_module_.remove_flit(i, j);
#if 1
					add_type cre_add_t = address_;
					long cre_pc_t = i;
					ConnectionDownStream( i,  address_, cre_add_t,  cre_pc_t, ary_size_);

					mess_queue::wm_pointer().add_message(
						mess_event(event_time + CREDIT_DELAY_, 
						CREDIT_, address_, cre_add_t, cre_pc_t, j));
#endif
					if(flit_t.type() == TAIL_) {
						if(input_module_.input(i, j).size() > 0) {
							input_module_.state_update(i, j, ROUTING_);
						}else {
							input_module_.state_update(i, j, INIT_);
						}
					}
				}
			}
		}
	}
}

//***************************************************************************//
