#include <exception>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "index.h"
#include "SStd.h"
#include "SRGen.h"
#include "configuration.h"
#include "sim_foundation.h"
#include "mess_queue.h"
extern "C" {
#include "SIM_power.h"
#include "SIM_router_power.h"
#include "SIM_power_router.h"
#include "../host.h"
#include "../machine.h"
#include "../network.h"
#include "../options.h"
#define MULTI_VC

int mainPopnet(int array_size);
void popnet_options(struct opt_odb_t *odb);
int popnetRunSim(long long int sim_cycle);
//void popnetMessageInsert(long s1, long s2, long d1, long d2, long long int sim_cycle, long size, counter_t msgNo, md_addr_t addr, int operation, int vc);
int popnetMsgComplete(long w, long x, long y, long z, long long int stTime, long long int msgNo);
int popnetBufferSpace(long s1, long s2, int opt);
void power_report(FILE *fd);
void delay_report(FILE *fd);
extern unsigned long long sim_cycle;
}

mess_queue *network_mess_queue_ptr;
sim_foundation *sim_net_ptr;
configuration *c_par_ptr;
SRGen *random_gen_ptr;
int msgCompleteCount;
long long int messageQueued = 0;
char *mesh_input_buffer_size;
char *mesh_output_buffer_size;
char *mesh_flit_size;
char *vc_num;
char *routing_algr;
int link_width;

void popnet_options(struct opt_odb_t *odb)
{
    opt_reg_string (odb, "-mesh_network:buffer_size", "", &mesh_input_buffer_size, /* default */"12", 
		    /* print */ TRUE, /* format */ NULL);
    opt_reg_string (odb, "-mesh_network:outbuffer_size", "", &mesh_output_buffer_size, /* default */ "1", 
		    /* print */ TRUE, /* format */ NULL);
    opt_reg_string (odb, "-mesh_network:mesh_flit_size", "", &mesh_flit_size, /* default */"64", 
		    /* print */ TRUE, /* format */ NULL);
    opt_reg_string (odb, "-mesh_network:vc_num", "", &vc_num, /* default */"1", 
		    /* print */ TRUE, /* format */ NULL);
    opt_reg_string (odb, "-mesh_network:routing_algr", "", &routing_algr, /* default */"0", 
		    /* print */ TRUE, /* format */ NULL);
    opt_reg_int (odb, "-mesh_network:phit_size", "", &link_width, /* default */64, 
		    /* print */ TRUE, /* format */ NULL);
}
void my_itoa(int n, char **s)
{
	switch (n)
	{
		case 1: (*s) = "1"; break;
		case 2: (*s) = "2"; break;
		case 3: (*s) = "3"; break;
		case 4: (*s) = "4"; break;
		case 5: (*s) = "5"; break;
		case 6: (*s) = "6"; break;
		case 7: (*s) = "7"; break;
		case 8: (*s) = "8"; break;
		case 9: (*s) = "9"; break;
		case 10: (*s) = "10"; break;
		case 11: (*s) = "11"; break;
		case 12: (*s) = "12"; break;
		default: (*s) = "6";
	}
	return;
}
int mainPopnet(int array_size)
{
	char *arg[23];
	char temp[23][32];
	char *my_array_size;
	my_itoa(array_size, &my_array_size);
	strcpy(temp[0], "popnet");
	strcpy(temp[1], "-A");
	strcpy(temp[2], my_array_size);
	strcpy(temp[3], "-c");
	strcpy(temp[4], "2");
	strcpy(temp[5], "-V");
	strcpy(temp[6], vc_num);
	strcpy(temp[7], "-B");
	strcpy(temp[8], mesh_input_buffer_size);
	strcpy(temp[9], "-O");
	strcpy(temp[10], mesh_output_buffer_size);
	strcpy(temp[11], "-F");
	strcpy(temp[12], mesh_flit_size);
	strcpy(temp[13], "-L");
	strcpy(temp[14], "1000");
	strcpy(temp[15], "-T");
	strcpy(temp[16], "20000");
	strcpy(temp[17], "-r");
	strcpy(temp[18], "1");
	strcpy(temp[19], "-I");
	strcpy(temp[20], "bench");
	strcpy(temp[21], "-R");
	strcpy(temp[22], routing_algr);

	arg[0] = temp[0];
	arg[1] = temp[1];
	arg[2] = temp[2];
	arg[3] = temp[3];
	arg[4] = temp[4];
	arg[5] = temp[5];
	arg[6] = temp[6];
	arg[7] = temp[7];
	arg[8] = temp[8];
	arg[9] = temp[9];
	arg[10] = temp[10];
	arg[11] = temp[11];
	arg[12] = temp[12];
	arg[13] = temp[13];
	arg[14] = temp[14];
	arg[15] = temp[15];
	arg[16] = temp[16];
	arg[17] = temp[17];
	arg[18] = temp[18];
	arg[19] = temp[19];
	arg[20] = temp[20];
	arg[21] = temp[21];
	arg[22] = temp[22];
	
		 
	 	
	cout << "in here: C++ program" << endl;
	try {
		random_gen_ptr = new SRGen;
		c_par_ptr = new configuration(23, arg);//argc, argv);
		network_mess_queue_ptr = new mess_queue(0.0);
		sim_net_ptr = new sim_foundation;
		

	} catch (exception & e) {
		cerr << e.what();
	}
}

int popnetRunSim(long long int sim_cycle)
{

	msgCompleteCount = 0;

	network_mess_queue_ptr->simulator(sim_cycle);
	
	if(msgCompleteCount > 0)
		cout << "Messages completed this cycle!!!!!" << endl;
	return 0;
}

//void popnetMessageInsert(long s1, long s2, long d1, long d2, long long int sim_cycle, long size, long long int msgNo, long long addr)
void popnetMessageInsert(long s1, long s2, long d1, long d2, long long int sim_cycle, long size, counter_t msgNo, md_addr_t addr, int operation, int vc)
{
	if(addr < 0)
	    addr = addr * -1;
	//if(msgNo == 7)
	cout << "wxh debug New message inserted" << sim_cycle << " " << s1 << " " << s2 << " " << d1 << " " << d2 << " Msg No" << msgNo << endl;
	//network_mess_queue_ptr->insertMsg(s1, s2, d1, d2, sim_cycle, size, msgNo, addr);
	network_mess_queue_ptr->insertMsg(s1, s2, d1, d2, sim_cycle, size, msgNo, addr, (long)vc);
	messageQueued++;
}

int popnetBufferSpace(long s1, long s2, int opt)
{
	add_type sor_addr_t;
	sor_addr_t.resize(2);
	sor_addr_t[0] = s1;
	sor_addr_t[1] = s2;
//	return sim_foundation::wsf().router(sor_addr_t).isBufferFull();
#ifdef MULTI_VC
	if(opt == -1)
		return sim_foundation::wsf().router(sor_addr_t).suggestVC();
	return sim_foundation::wsf().router(sor_addr_t).isBufferFull(0, opt) ? -1:opt;
#else
	return sim_foundation::wsf().router(sor_addr_t).isBufferFull(0, 0);
#endif
}

int finishedMessage(long w, long x, long y, long z, long long int stTime, long long int msgNo)
{
//  if(msgNo == 7)
  cout << "wxh debug packet received" << stTime << " " << w << " " << x << " " << y << " " << z << " Msg No" << msgNo << endl;
	if(popnetMsgComplete(w, x, y, z, stTime, msgNo))
	{
		messageQueued--;
		return 1;
	}
	return 0;
}
void delay_report(FILE *fd)
{
	vector<sim_router_template>::const_iterator first = 
							sim_foundation::wsf().inter_network().begin();
	vector<sim_router_template>::const_iterator last = 
							sim_foundation::wsf().inter_network().end();
	double total_delay = 0;
	//calculate the total delay
	first = sim_foundation::wsf().inter_network().begin();
	for(; first != last; first++) {
		total_delay += first->total_delay();
	}
	long tot_f_t = mess_queue::wm_pointer().total_finished();

	fprintf(fd,"total finished:       %g\n",tot_f_t);
	fprintf(fd,"Total delay in popNet: %g\n", total_delay/tot_f_t);
}

void power_report(FILE *fd)
{
    sim_foundation::wsf().simulation_results();
    double total_power = 0;
    for(int i = 0; i < sim_foundation::wsf().ary_size(); i++)
    {
	for(int j = 0; j < sim_foundation::wsf().ary_size(); j++)
	{
	    add_type sor_addr_t;
	    sor_addr_t.resize(2);
	    sor_addr_t[0] = i;
	    sor_addr_t[1] = j;
	    //total_power += sim_foundation::wsf().router(sor_addr_t).power_report(fd, sim_foundation::wsf().ary_size()*sim_foundation::wsf().ary_size());
	    total_power += sim_foundation::wsf().router(sor_addr_t).power_report(fd, sim_foundation::wsf().ary_size()*(sim_foundation::wsf().ary_size()-2));
	}
    }
    fprintf(fd,"Total Network power: %g\n", total_power);
}
