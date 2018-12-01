#ifndef LOGIN_MODUELS_H__
#define LOGIN_MODUELS_H__

// 返回用户的uid
enum {
	STATUS_SUCCESS = 0,
	STATUS_ERROR = -1,
};

struct uinfo {
	unsigned int uid;
	char unick[64];
	int is_guest;
};

int query_guest_login(char* ukey, struct uinfo* out_info);
#endif