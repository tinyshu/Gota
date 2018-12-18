#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "../../utils/log.h"
#include "../../utils/timer.h"
#include "../../utils/timer_list.h"
}

#include "../../moduel/net/net_uv.h"
#include "server_session_mgr.h"
#include "gw_config.h"



#define MAX_SERVER_COUNT 16
//extern struct session* netbus_connect(char* server_ip, int port, int stype);

struct SESSION_MGR {
	int need_connectd; //是否连接成功
	struct session* server_session[MAX_SERVER_COUNT];
}SESSION_MGR;

void init_server_session() {
	memset(&SESSION_MGR,0,sizeof(SESSION_MGR));
	SESSION_MGR.need_connectd = 1;
	NETBUS_TIMER_LIST = create_timer_list();
#ifdef GAME_DEVLOP
	//单进程模式不
	LOGINFO("single dev moduel\n");
#else
	//启动定时器
	netbus_schedule(check_server_online, NULL, 1);
#endif
	
}

void destroy_session_mgr() {
	destroy_timer_list(NETBUS_TIMER_LIST);
}

struct session* get_server_session(int stype) {
	if (stype < 0 || stype > MAX_SERVER_COUNT) {
		LOGERROR("get_server_session error stype:%d\n", stype);
		return NULL;
	}

	return SESSION_MGR.server_session[stype];
}

void netbus_schedule(void(*on_time)(void* data), void* kdata, float after_sec) {
	schedule_timer(NETBUS_TIMER_LIST, on_time, kdata, after_sec);
}

void check_server_online(void* data) {
	if (0 == SESSION_MGR.need_connectd) {
		return;
	}
	SESSION_MGR.need_connectd = 0;
	//for (int i = 0; i < GW_CONFIG.num_server_moudle;++i) {
	for (int i = 0; i < 1; ++i) {
		int stype = GW_CONFIG.module_set[i].stype;
		if (NULL == SESSION_MGR.server_session[stype]) {
			SESSION_MGR.server_session[stype] = netbus_connect(GW_CONFIG.module_set[i].ip, GW_CONFIG.module_set[i].port);
			if (NULL == SESSION_MGR.server_session[stype]) {
				//连接失败了
				printf("connect %s is faild ip:%s port:%d\n", GW_CONFIG.module_set[i].desic, GW_CONFIG.module_set[i].ip, GW_CONFIG.module_set[i].port);
				SESSION_MGR.need_connectd = 1;
			}
			else {
					printf("connect %s is sucess ip:%s port:%d\n", GW_CONFIG.module_set[i].desic, GW_CONFIG.module_set[i].ip, GW_CONFIG.module_set[i].port);					
			}
		}
	}
}

void lost_server_connection(int stype) {
	if (stype < 0 || stype > STYPE_MAX_OFFSET) {
		return;
	}
	SESSION_MGR.server_session[stype] = NULL;
	SESSION_MGR.need_connectd = 1; //设置重新连接
}

