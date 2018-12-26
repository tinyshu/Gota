#ifndef EXPORT_UDP_SESSION_H__
#define EXPORT_UDP_SESSION_H__

#include "export_session.h"
struct recv_msg;
struct session;
struct udp_session;
struct session_base;

class export_udp_session :public export_session {
public:
	virtual void close();
	virtual void send_data(unsigned char* pkg, int pkg_len);
	virtual void send_msg(recv_msg* msg);
	session_base* get_inner_session();
public:
	session_base* _udp_session;
};

#endif