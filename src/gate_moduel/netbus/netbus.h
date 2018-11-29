#ifndef NETBUS_H__
#define NETBUS_H__

#include "../../3rd/mjson/json.h"
extern struct timer_list* GATEWAY_TIMER_LIST;

struct service_module  {
	int stype;
	//注册
	void (*init_service_module)(struct service_module* module);
	//处理二进制协议接口
	int (*on_bin_protocal_data)(void* moduel_data,struct session* s,unsigned char* pkg,int len);

	//json协议处理接口
	int (*on_json_protocal_data)(void* moduel_data, struct session* s,json_t* root, unsigned char* pkg, int len);

	void (*on_connect_lost)(void* module_data, struct session* s);
	//关闭接口
	void* moduel_data;
};

extern void exit_server_gateway();
extern void init_server_gateway();
extern void on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len);
extern void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);

void register_services(int stype, struct service_module* module);

#endif