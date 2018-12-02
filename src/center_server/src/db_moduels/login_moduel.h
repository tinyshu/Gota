#ifndef LOGIN_MODUELS_H__
#define LOGIN_MODUELS_H__

// db_modyels模块下都是操作mysql和redis函数定义和实现
// 返回用户的uid
enum {
	STATUS_SUCCESS = 0,
	STATUS_ERROR = -1,
};

struct user_info {
	unsigned int uid;
	char unick[64];
	int is_guest;
};

int query_guest_login(char* ukey, struct user_info* out_info);
#endif