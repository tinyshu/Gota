#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../utils/log.h"
#include "../../types_service.h"
#include "../../command.h"
#include "../../error_command.h"
#include "../../moduel/netbus/netbus.h"
#include "../../3rd/mjson/json_extends.h"
#include "center_services.h"

///////////////////////////////////////////////////////////////
static void guest_login(void* moduel_data, struct session* s, 
	json_t* root,unsigned int uid,unsigned int skey) {

	int para_count = json_object_size(root);
	if (3 != para_count) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS,"parament count error",uid,skey);
		return;
	}

	json_t* client_rand_key = json_object_at(root,"2");
	if (NULL!=client_rand_key || client_rand_key->type) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client rand key",uid, skey);
		return;
	}

	//使用key在db里查找
	
}
///////////////////////////////////////////////////////////////
static void init_service_module(struct service_module* module) {
	
}

//json协议处理接口
int on_center_json_protocal_data(void* moduel_data, struct session* s, json_t* root, unsigned char* pkg, int len) {
	//printf("on_center_json_protocal_data\n");
	json_t* jcmd = json_object_at(root, "1");
	if (jcmd == NULL || jcmd->type != JSON_NUMBER) {
		return 0;
	}
	int cmd = atoi(jcmd->text);
	json_t* juid = json_object_at(root, "uid");
	json_t* jskey = json_object_at(root,"skey");
	unsigned int uid = strtoul(juid->text, NULL, 10);
	unsigned int skey = strtoul(jskey->text, NULL, 10);
	// 转给上层业务模块处理
	switch (cmd) {
	case GUEST_LOGIN: {
		guest_login(moduel_data,s, root,uid,skey);
	}break;

	}

	//回发给gateway
	json_t* ret = json_new_comand((SYPTE_CENTER + TYPE_OFFSET), cmd);
	json_object_push_number(ret, "2", 1);
#ifndef GAME_DEVLOP
	json_object_push_number(ret, "uid", strtoul(juid->text,NULL,10));
	json_object_push_number(ret, "skey", strtoul(jskey->text,NULL,10));
#endif
	session_json_send(s, ret);
	json_free_value(&ret);
	return 0;
}

void on_center_connect_lost(void* module_data, struct session* s) {
	LOGINFO("center_connect_lost\n");
}

struct service_module CENTER_SERVICE = {
	SYPTE_CENTER,
	init_service_module,   //初始化函数
	NULL,   //二进制协议
	on_center_json_protocal_data,   //json协议
	on_center_connect_lost,   //连接关闭
	NULL,   //使用自定义数据
};
