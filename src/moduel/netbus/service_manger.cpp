#include <string.h>
#include "service_manger.h"
#include "service_interface.h"
#include "recv_msg.h"

server_manage::server_manage() {
	memset(_services,0,sizeof(_services));
}

server_manage::~server_manage() {
	for (int i = 0; i < MAX_SERVICES; i++) {
		if (_services[i] != NULL) {
			delete _services[i];
			_services[i] = NULL;
		}
	}
}

server_manage& server_manage::get_instance() {
	static server_manage service_mgr;
	return service_mgr;
}

//网络层的消息先转到这里，在根据stype调用对应的service函数
bool server_manage::on_session_recv_cmd(struct session_base* s, recv_msg* msg) {
	if (msg==NULL || msg->stype == MAX_SERVICES || s==NULL) {
		return false;
	}

	bool ret = false;
	if (_services[msg->stype]!=NULL) {
		ret = _services[msg->stype]->on_session_recv_cmd(s,msg);
	}

	return ret;
}

void server_manage::on_session_disconnect(int service_type,struct session* s) {
	for (int i = 0; i < MAX_SERVICES; i++) {
		if (_services[i] == NULL) {
			continue;
		}

		_services[i]->on_session_disconnect(s);
	}
}

void server_manage::register_service(int service_type, service* s) {
	if (service_type <=0 || service_type >= MAX_SERVICES) {
		return;
	}

	_services[service_type] = s;
}