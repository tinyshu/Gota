#ifndef __SERVICE_INTERFACE_H__
#define __SERVICE_INTERFACE_H__

struct session;
struct recv_msg;
struct session_base;
struct raw_cmd;

class service {
public:
	service() { 
		stype = 0;
		using_direct_cmd = false;
	}
public:
	virtual bool on_session_recv_cmd(struct session_base* s, recv_msg* msg) = 0;
	virtual void on_session_disconnect(struct session* s) = 0;
	virtual bool on_session_recv_raw_cmd(struct session_base* s,raw_cmd* msg) = 0;
public:
	//service对应的stype
	int stype;
	//直接转发模式,为true用于网关二进制不解析数据,
	bool using_direct_cmd; 
};

#endif
