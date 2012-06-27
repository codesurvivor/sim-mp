#ifndef NETWORK_MESS_EVENT_H_
#define NETWORK_MESS_EVENT_H_

#include "index.h"
#include "flit.h"
#include <vector>
#include <iostream>
#include "RouterIDMap.h"


class mess_event {
	friend ostream & operator<<(ostream& os, const mess_event & sg);

	private:
		time_type time_;
		mess_type mess_;
		add_type src_;
		add_type des_;
		long pc_;
		long vc_;
		flit_template flit_;
		long pacSize;
	public:

		time_type event_start() {return time_;}
		time_type event_start() const {return time_;}
		mess_type event_type() {return mess_;}
		mess_type event_type() const {return mess_;}
		add_type & src() {return src_;}
		const add_type & src() const {return src_;}
		add_type & des() {return des_;}
		const add_type & des() const {return des_;}
		long pSize() {return pacSize;}
		
		long pc() {return pc_;}
		long pc() const {return pc_;}
		long vc() {return vc_;}
		long vc() const {return vc_;}
		flit_template & get_flit() {return flit_;}
		const flit_template & get_flit() const {return flit_;}

		//ROUTER_ message
		mess_event(time_type t, mess_type mt);
		//EVG_ message
                mess_event(time_type t, mess_type mt, const add_type & a, const add_type & b, long p);
 
		//CREDIT_ message
		mess_event(time_type t, mess_type mt, 
		 const add_type & a, const add_type & b, long c, long d);
		//WIRE_ message c is the phy and d is the vc
		mess_event(time_type t, mess_type mt, 
		 const add_type & a, const add_type & b, long c, long d,
		 const flit_template & f);
		mess_event(time_type t, mess_type mt, const flit_template & f);

		mess_event(mess_event& me);
		mess_event(const mess_event& me);
};
#endif
