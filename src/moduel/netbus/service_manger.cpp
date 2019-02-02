#include <string.h>
#include "service_manger.h"
#include "service_interface.h"
#include "recv_msg.h"
#include "../../proto/proto_manage.h"
#include "../../utils/mem_manger.h"
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
	if (msg==NULL || msg->head.stype >= MAX_SERVICES || s==NULL) {
		return false;
	}

	bool ret = false;
	if (_services[msg->head.stype]!=NULL) {
		ret = _services[msg->head.stype]->on_session_recv_cmd(s,msg);
	}

	return ret;
}

bool server_manage::on_recv_raw_cmd(struct session_base* s, struct raw_cmd* raw) {
	if (s==NULL || raw==NULL) {
		return false;
	}
	if (raw->head.stype >= MAX_SERVICES || _services[raw->head.stype] == NULL) {
		return false;
	}

	bool ret = false;
	//判断是否为网关直接转发模式
	if (_services[raw->head.stype]->using_direct_cmd == true) {
		//网关直接转发模式
		ret = _services[raw->head.stype]->on_session_recv_raw_cmd(s,raw);
	}
	else {
		//非网关需要解析数据body
		recv_msg* msg = NULL;
		if (true == protoManager::decode_cmd_msg(raw->raw_data, raw->raw_len,&msg)) {
			if (msg) {
				ret = server_manage::get_instance().on_session_recv_cmd(s, msg);
				memory_mgr::get_instance().free_memory(msg);
			}
		}
		
	}
	return ret;
}

void server_manage::on_session_disconnect(struct session_base* s) {
	for (int i = 0; i < MAX_SERVICES; i++) {
		if (_services[i] == NULL) {
			continue;
		}

		_services[i]->on_session_disconnect((session*)s);
	}
}

void server_manage::register_service(int service_type, service* s) {
	if (service_type <=0 || service_type >= MAX_SERVICES) {
		return;
	}

	_services[service_type] = s;
}