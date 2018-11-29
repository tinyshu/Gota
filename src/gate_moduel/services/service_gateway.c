#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "service_gateway.h"

#define MAX_SERVICES 512
#define my_free free;

struct timer_list* GATEWAY_TIMER_LIST = NULL;

extern void close_session(struct session* s);
extern void session_send(struct session*s, unsigned char* body, int len);

//网关负责转发的后台service模块
struct {
	struct service_module* services[MAX_SERVICES];
}gateway_services;

void register_services(int stype, struct service_module* module) {
	if (stype <= 0 || stype >= MAX_SERVICES || NULL == module) {
		printf("register error %d", stype);
		return;
	}

	gateway_services.services[stype] = module;
	if (module->init_service_module) {
		module->init_service_module(module);
	}
	
}



void init_server_gateway() {
	memset(&gateway_services, 0, sizeof(gateway_services));
}

void exit_server_gateway() {
	//释放内存
	for (int i = 0; i < MAX_SERVICES; ++i) {
		struct service_module* moduel = gateway_services.services[i];
		if (NULL!= moduel) {
			my_free(moduel);
		}
	}
}

//底层如果发现底层网络连接断开，通知给上层的接口
//该接口通知关心的模块
void on_connect_lost(struct session* s) {
	for (int i = 0; i < MAX_SERVICES;++i) {
		if (gateway_services.services[i] && gateway_services.services[i]->on_connect_lost != NULL) {
			gateway_services.services[i]->on_connect_lost(gateway_services.services[i]->moduel_data,s);
		}
	}
}

//二进制协议处理,由网络底层解析到一个完整的package后调用
void on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	int stype = ((data[0]) | (data[1] << 8) | (data[2] << 16) | (data[3] << 24));
	if (gateway_services.services[stype] && gateway_services.services[stype]->on_bin_protocal_data) {
		int ret = gateway_services.services[stype]->on_bin_protocal_data(gateway_services.services[stype]->moduel_data,s, data, len);
		if (ret < 0) {
			close_session(s);
		}
	}
#if _DEBUG
	printf("stype:%d", stype);
#endif
}

//json协议格式处理，由网络底层解析到一个完整的package后调用
void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len) {
	if (0 == len || NULL == data) {
		return;
	}
	data[len] = 0;
	json_t* root = NULL;
	int ret = json_parse_document(&root, data);
	if (ret != JSON_OK || root == NULL) { // 不是一个正常的json包;
		return;
	}

	json_t* server_type = json_find_first_label(root, "0");
	server_type = server_type->child;
	if (server_type == NULL || server_type->type != JSON_NUMBER ) {
		json_free_value(&root);
		return;
	}
	int stype = atoi(server_type->text);
#ifdef _DEBUG
	printf("stype:%d", stype);
#endif
	if (gateway_services.services[stype] && gateway_services.services[stype]->on_json_protocal_data) {
		gateway_services.services[stype]->on_json_protocal_data(gateway_services.services[stype]->moduel_data,s, root,data,len);
	}

	json_free_value(&root);
}