#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "login_moduel.h"
#include "../../../database/center_db.h"

int query_guest_login(char* rand_key, char* unick, unsigned int usex, unsigned int uface, struct user_info* out_info) {

	if (0 == get_userinfo_buy_key(rand_key, out_info)) {
		if (0 == out_info->is_guest) {
			//不是游客
			return MODEL_ACCOUNT_IS_NOT_GUEST;
		}

		return MODEL_SUCCESS;
	}

	//插入数据
	if (insert_guest_with_key(rand_key, unick, uface, usex) == 0) {
		if (0 == get_userinfo_buy_key(rand_key, out_info)) {
			return MODEL_SUCCESS;
		}
		
	}
	return MODEL_SYSTEM_ERR;
}