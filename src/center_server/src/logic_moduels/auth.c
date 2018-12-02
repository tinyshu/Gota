#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "auth.h"

#include "../../../types_service.h"
#include "../../../command.h"
#include "../../../error_command.h"
#include "../db_moduels/login_moduel.h"
#include "../../../database/center_db.h"
/*
请求:
0:服务类型 stype
1:命名类型 cmd
2:ukey 
3:unick
4:usex
5:uface

响应
status,  --OK
unick,
uface,
usex.
*/
void guest_login(void* moduel_data, struct session* s,
	json_t* root, unsigned int uid, unsigned int skey) {
	
	int para_count = json_object_size(root);
	if (6+2 != para_count) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "parament count error", uid, skey);
		return;
	}

	char* client_rkeys = json_object_get_string(root,"2");
	if (NULL == client_rkeys) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client rand key", uid, skey);
		return;
	}

	char* user_nick = json_object_get_string(root, "3");
	if (NULL == user_nick) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client nick name", uid, skey);
		return;
	}

	unsigned int user_sex = json_object_get_unsigned_number(root, "4");
	unsigned int user_face = json_object_get_unsigned_number(root, "5");
	//使用key在db里查找
	struct user_info user_info;
	int ret = query_guest_login(client_rkeys,user_nick,user_sex,user_face, &user_info);
	if (MODEL_ACCOUNT_IS_NOT_GUEST == ret) {
		write_error(s, SYPTE_CENTER, GUEST_LOGIN, ACCOUNT_IS_NOT_GUEST,"client is not guest", uid,client_rkeys);
		return;
	}

	if (MODEL_SUCCESS != ret) {
		write_error(s, SYPTE_CENTER, GUEST_LOGIN, SYSTEM_ERROR, "process guest_login error", uid, client_rkeys);
		return;
	}

	json_t* send_json = json_new_server_return_cmd(SYPTE_CENTER, GUEST_LOGIN, user_info.uid, skey);
	json_object_push_number(send_json, "2",OK);
	json_object_push_string(send_json,"3", user_info.unick);
	json_object_push_number(send_json,"4",user_info.uface);
	json_object_push_number(send_json, "5", user_info.usex);
	
	session_json_send(s, send_json);
	json_free_value(&send_json);

}