#ifndef __SERVICE_INTERFACE_H__
#define __SERVICE_INTERFACE_H__

struct session;
struct recv_msg;
struct session_base;

class service {
public:
	virtual bool on_session_recv_cmd(struct session_base* s, recv_msg* msg) = 0;
	virtual void on_session_disconnect(struct session* s) = 0;
public:
	int stype;
};

#endif
