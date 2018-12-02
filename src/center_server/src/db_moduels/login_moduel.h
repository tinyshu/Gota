#ifndef LOGIN_MODUELS_H__
#define LOGIN_MODUELS_H__

// db_modyels模块下都是操作mysql和redis函数定义和实现
// 返回用户的uid
enum {
	MODEL_SUCCESS = 0,
	STATUS_ERROR = -1,
	MODEL_ACCOUNT_IS_NOT_GUEST=-2,
	MODEL_SYSTEM_ERR=-3,
};

int query_guest_login(char* rand_key, char* unick, unsigned int usex, unsigned int uface, struct user_info* out_info);
#endif