#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "auth.h"

#include "../../../types_service.h"
#include "../../../command.h"
#include "../../../error_command.h"
#include "../db_moduels/login_moduel.h"
/*
0:服务类型 stype
1:命名类型 cmd
3:ukey
*/
void guest_login(void* moduel_data, struct session* s,
	json_t* root, unsigned int uid, unsigned int skey) {

	int para_count = json_object_size(root);
	if (3 != para_count) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "parament count error", uid, skey);
		return;
	}

	char* client_rkeys = json_object_get_string(root,"2");
	if (NULL == client_rkeys) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client rand key", uid, skey);
		return;
	}

	//使用key在db里查找
	struct user_info user_info;
	query_guest_login(client_rkeys,&user_info);
}