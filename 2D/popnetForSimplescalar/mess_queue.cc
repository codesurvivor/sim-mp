#include "mess_queue.h"
#include "sim_foundation.h"
#include <string>
#include <iostream>
#include <strstream>
#include <unistd.h>


bool operator<(const mess_event & a, const mess_event & b) {
	return a.event_start() < b.event_start();
}

mess_queue * mess_queue::m_pointer_ = 0;

mess_queue::mess_queue(time_type start_time):
    current_time_(0),
	last_time_(0),
	mess_counter_(0),
	m_q_(),
	total_finished_(0)
{
	current_time_ = start_time;
	m_pointer_ = this;
	add_message(mess_event(0, ROUTER_));
}

string mess_queue:: mess_error_ = 
	string("This message type is not supported.\n");

	
	
//void mess_queue::insertMsg(int src1, int src2, int dest1, int dest2, long long int sim_cycle, long packetSize, long long int msgNo, long long addr)
void mess_queue::insertMsg(int src1, int src2, int dest1, int dest2, long long int sim_cycle, long packetSize, long long int msgNo, long long addr, long vc)
{

	add_type sor_addr_t;
	add_type des_addr_t;
	time_type event_time_t;

	event_time_t = sim_cycle;

	
	sor_addr_t.resize(4);
	des_addr_t.resize(4);
	sor_addr_t[0] = src1;
	sor_addr_t[1] = src2;
	des_addr_t[0] = dest1;
	des_addr_t[1] = dest2;
	
	sor_addr_t[2] = packetSize;
	des_addr_t[2] = msgNo;

	sor_addr_t[3] = addr;
	des_addr_t[3] = vc;
	if(packetSize > 10)
		cout << "some wrong\n" <<endl;
	mess_queue::wm_pointer().add_message(mess_event(event_time_t, EVG_, sor_addr_t, des_addr_t, packetSize));

}

void mess_queue::simulator(long long int sim_cycle) {

	time_type report_t = 0;
	long total_incoming = 0;
	while(m_q_.size() > 0)
	{
		mess_event current_message = * get_message();
		current_time_ = current_message.event_start();
		
		
		if(current_time_ > sim_cycle)
			return;
		
			
		remove_top_message();
		
		switch(current_message.event_type()) {

			case EVG_ :
				sim_foundation::wsf().receive_EVG_message(current_message);
				total_incoming ++;
			break;

			case ROUTER_ :
				sim_foundation::wsf().receive_ROUTER_message(current_message);
			break;

			case WIRE_ :
				sim_foundation::wsf().receive_WIRE_message(current_message);
			break;

			case CREDIT_ :
				sim_foundation::wsf().receive_CREDIT_message(current_message);
			break;

			default:
				throw pro_error(mess_error_);
			break;
		} 
	}
}

