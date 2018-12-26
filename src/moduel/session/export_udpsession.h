#ifndef EXPORT_UDP_SESSION_H__
#define EXPORT_UDP_SESSION_H__

#include "export_session.h"
struct recv_msg;
struct session;

class export_udp_session :public export_session {
public:
	virtual void close();
	virtual void send_data(unsigned char* pkg, int pkg_len);
	virtual void send_msg(recv_msg* msg);
	session* get_inner_session();
public:
	
};

#endif