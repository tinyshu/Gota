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

//逻辑模块入口
#include "logic_moduels/auth.h"
///////////////////////////////////////////////////////////////

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
//	json_t* ret = json_new_comand((SYPTE_CENTER + TYPE_OFFSET), cmd);
//	json_object_push_number(ret, "2", 1);
//#ifndef GAME_DEVLOP
//	json_object_push_number(ret, "uid", strtoul(juid->text,NULL,10));
//	json_object_push_number(ret, "skey", strtoul(jskey->text,NULL,10));
//#endif
//	session_json_send(s, ret);
//	json_free_value(&ret);
	return 0;
}

void on_center_connect_lost(void* module_data, struct session* s) {
#ifndef GAME_DEVLOP
	//多进程部署模式，s为gateway连接
	LOGINFO("center_connect_lost\n");
#else
	//单进程调试方式，s是前端连接的client
#endif // !GAME_DEVLOP

	
}

struct service_module CENTER_SERVICE = {
	SYPTE_CENTER,
	init_service_module,   //初始化函数
	NULL,   //二进制协议
	on_center_json_protocal_data,   //json协议
	on_center_connect_lost,   //连接关闭
	NULL,   //使用自定义数据
};
