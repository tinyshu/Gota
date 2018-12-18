#ifndef __SERVICE_MANAGE_H__
#define __SERVICE_MANAGE_H__

#include "netbus.h"

struct session;
class service;
struct recv_msg;

class server_manage {
public:
	server_manage();
	~server_manage();

	static server_manage& get_instance();
	
	void register_service(int service_type, service* s);
	bool on_session_recv_cmd(struct session* s, recv_msg* msg);
	void on_session_disconnect(int service_type,struct session* s);
private:
	static server_manage* _instance;
	service* _services[MAX_SERVICES];
};


#endif
