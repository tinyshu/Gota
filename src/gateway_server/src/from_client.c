#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "from_client.h"
#include "../../types_service.h"
#include "../../protocal_cmd.h"
#include "../../gate_moduel/services/service_gateway.h"
#include "server_session_mgr.h"
#include "../../3rd/mjson/json_extends.h"
#include "session_key_mgr.h"
#define my_malloc malloc

//收到客户端json数据后,网络层调用service_gateway模块on_json_protocal_data,
//service_gateway在根据stype调用到这里
//该函数处理消息在转发到对应的后端服务
static int on_json_protocal_data (void* moduel_data, struct session* s, json_t* root, 
	unsigned char* pkg, int len){
	int stype = (int)(moduel_data);
	
	struct session* server_session = get_server_session(stype);
	if (NULL == server_session) {
		//stype不正确或者是连接的后端服务断开了连接
		return 0;
	}

	//获取用户uid并做权限验证
	unsigned int uid = 123;
	//获取一个随机key和session绑定
	unsigned int session_key = get_session_key();
	save_session_by_key(session_key,s);
	json_object_push_number(root,"uid",uid);
	json_object_push_number(root,"key", session_key); //后端服务需要透明传回这个值
	session_json_send(server_session, root);
	return 0;
}

//当网络底层检测到客户端连接的session关闭，由网络层通知到该函数
static void on_connect_lost (void* moduel_data, struct session* s) {
	int stype = (int)moduel_data;
	//需要通知后端的服务,有客户端session断开
	unsigned int uid = s->uid;
	
	struct session* server_session = get_server_session(stype);
	if (server_session == NULL || uid == 0) { // gateway与服务所在进程断开了网络连接
		//不是一个client类型的session
		return;
	}
	json_t* json = json_new_comand(stype, USER_LOST_CONNECT);
	json_object_push_number(json, "2", uid);

	session_json_send(server_session, json);
	json_free_value(&json);

	//清除hash_map_int里的玩家session指针
	clear_session_by_value(s); 
	
}

void register_from_client_moduel(int stype) {
	struct service_module* register_moduel = (struct service_module*)my_malloc(sizeof(struct service_module));
	if (NULL == register_moduel) {
		exit(-1);
	}

	register_moduel->init_service_module = NULL;
	register_moduel->on_bin_protocal_data = NULL;
	register_moduel->on_json_protocal_data = on_json_protocal_data;
	register_moduel->on_connect_lost = on_connect_lost;
	register_moduel->moduel_data = (void*)stype;

	register_services(stype, register_moduel);
}