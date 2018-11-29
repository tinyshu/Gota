#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "return_server.h"
#include "../../utils/log.h"
#include "../../types_service.h"
#include "../../gate_moduel/services/service_gateway.h"
#include "server_session_mgr.h"
#include "../../3rd/mjson/json_extends.h"

#define my_malloc malloc

static unsigned int get_json_skey(json_t* root) {
	if (NULL==root) {
		return 0;
	}
	unsigned int skey = 0;
	json_t* jvalue = json_object_at(root, "skey");
	if (NULL == jvalue || jvalue->type != JSON_NUMBER) {
		return 0;
	}

	skey = atoll(jvalue->text);
	return skey;
}

static unsigned int get_json_uid(json_t* root) {
	
	if (NULL == root) {
		return 0;
	}
	unsigned int uid = 0;
	json_t* jvalue = json_object_at(root, "skey");
	if (NULL == jvalue || jvalue->type != JSON_NUMBER) {
		return 0;
	}

	uid = atoll(jvalue->text);
	return uid;
}

static int on_json_protocal_data(void* moduel_data, struct session* s, json_t* root,
	unsigned char* pkg, int len) {
	int stype = (int)moduel_data;

#ifdef _DEBUG
	char* str = NULL;
	json_tree_to_string(root,&str);
	printf("on_json_protocal_data %s\n", str);
	json_free_str(str);
#endif
	unsigned int session_uid = get_json_uid(root);
	//通过透传的rand_key找找到client_session
	unsigned int skey = get_json_skey(root);
	struct session* client_session = get_session_by_key(skey);
	if (NULL == client_session || session_uid <= 0) {
		//没有找到客户端，可能客户端已经关闭连接
		LOGINFO("not found client session is lost uid:%u\n", session_uid);
		return 0;
	}

	int client_stype = stype;
	if (client_stype <=0 || client_stype > STYPE_MAX_OFFSET) {
		LOGINFO("client_stype error %d", client_stype);
		return 0;
	}
	//验证用户是否登录
	//check_auth();
	//给客户端发送数据
	json_object_update_number(root,"0", client_stype);
	json_object_remove(root,"skey");
	json_object_remove(root, "uid");
#ifdef _DEBUG
	char* text = NULL;
	json_tree_to_string(root,&text);
	printf("on_json_protocal_data: %s\n", text);
	json_free_str(text);
#endif
	session_json_send(client_session, root);
	return 0;
}

static void on_connect_lost(void* moduel_data, struct session* s) {
	int stype = (int)moduel_data;
	//后端里连接的服务器断线
	if (1 == s->is_server_session) {
		lost_server_connection(stype);
	}
	
}


void register_server_return_moduel(int stype) {
	struct service_module* register_moduel = (struct service_module*)my_malloc(sizeof(struct service_module));
	if (NULL == register_moduel) {
		exit(-1);
	}

	int return_stype = stype + TYPE_OFFSET;
	register_moduel->stype = stype;
	register_moduel->init_service_module = NULL;
	register_moduel->on_bin_protocal_data = NULL;
	register_moduel->on_json_protocal_data = on_json_protocal_data;
	register_moduel->on_connect_lost = on_connect_lost;
	register_moduel->moduel_data = stype;

	register_services(return_stype, register_moduel);
}

