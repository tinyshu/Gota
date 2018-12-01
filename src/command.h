#ifndef COMMAND_H__
#define COMMAND_H__

#include "3rd/mjson/json.h"
#include "./3rd/mjson/json_extends.h"

extern const int CENTER_TYPE;

void json_get_uid_and_key(json_t* cmd, unsigned int* uid, unsigned int* s_key);
json_t* json_new_server_return_cmd(int stype, int cmd,unsigned int uid,unsigned int s_key);
void write_error(struct session* s, int stype,
	int cmd, int status,char* errmsg,
	unsigned int uid, unsigned int s_key);
//共享
enum {
	USER_LOST_CONNECT = 1,
	MAX_COMMON_NUM,
};

//中心服务器协议
enum {
	GUEST_LOGIN = MAX_COMMON_NUM, // 游客登陆
	USER_LOGIN, // 用户密码登录
	EDIT_PROFILE, // 修改用户资料
};

//通用服务器协议
//游戏好友服务器
//游戏熟人服务器

#endif