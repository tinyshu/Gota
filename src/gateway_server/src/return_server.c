#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "return_server.h"
#include "../../types_service.h"
#include "../../gate_moduel/services/service_gateway.h"
#include "server_session_mgr.h"
#include "../../3rd/mjson/json_extends.h"

#define my_malloc malloc


static int on_json_protocal_data(void* moduel_data, struct session* s, json_t* root,
	unsigned char* pkg, int len) {
	int stype = (int)moduel_data;

	struct session* client_session = NULL;
#ifdef _DEBUG
	char* str = NULL;
	json_tree_to_string(root,&str);
	printf("on_json_protocal_data %s\n", str);
	json_free_str(str);
#endif
	json_t* jvalue = json_object_at(root,"uid");
	if (NULL == jvalue || jvalue->type != JSON_NUMBER) {
		return 0;
	}
	//通过uid找到客户端对应的session
	unsigned int session_uid = atoll(jvalue->text);
	//get session
	if (NULL == client_session || session_uid <= 0) {
		//没有找到客户端，可能客户端已经关闭连接
		return 0;
	}

	int client_stype = stype - TYPE_OFFSET;
	
	//验证用户是否登录
	//发送数据
	session_json_send(client_session, root);
	return 0;
}

static void on_connect_lost(void* moduel_data, struct session* s) {
	int stype = (int)moduel_data;
	//后端里连接的服务器断线
	lost_server_connection(stype);
}


void register_server_return_moduel(int stype) {
	struct service_module* register_moduel = (struct service_module*)my_malloc(sizeof(struct service_module));
	if (NULL == register_moduel) {
		exit(-1);
	}

	register_moduel->init_service_module = NULL;
	register_moduel->on_bin_protocal_data = NULL;
	register_moduel->on_json_protocal_data = on_json_protocal_data;
	register_moduel->on_connect_lost = on_connect_lost;
	register_moduel->moduel_data = (void*)stype;

	register_services(stype + TYPE_OFFSET, register_moduel);
}

