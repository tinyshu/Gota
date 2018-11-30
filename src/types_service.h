#ifndef TABLE_TYPE_SERVICE_H__
#define TABLE_TYPE_SERVICE_H__

//服务模块[0, 255], [256, 511]
//[0, 255]是from_client模块注册的服务模块
//[256, 511]是return_server注册模块
#ifdef GAME_DEVLOP
#define TYPE_OFFSET 0
#else
#define TYPE_OFFSET 0xff
#endif

#define STYPE_MAX_OFFSET 512
#define SERVER_SESSION_OFFSET (TYPE_OFFSET +1)
//这个type是客户端到后台服务器的stype,有网关转发
enum {
    SYPTE_CENTER = 1, // 用户数据，登陆，资料服务
	STYPE_SYSTEM = 2, // 任务，邮件，奖励，排行
	STYPE_GAME1 = 2, // 网络对战
	STYPE_GAME2 = 3, // 好友对战
	STYPE_MAX = 255,
};

#endif