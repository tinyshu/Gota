#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server_session_mgr.h"
#include "gw_config.h"

#define MAX_SERVER_COUNT 16
struct SESSION_MGR {
	int need_connectd; //是否连接成功
	struct session* server_session[MAX_SERVER_COUNT];
}SESSION_MGR;

void init_server_session() {
	memset(&SESSION_MGR,0,sizeof(SESSION_MGR));
	SESSION_MGR.need_connectd = 1;
	GATEWAY_TIMER_LIST = create_timer_list();
	//启动定时器
	gateway_schedule(check_server_online,NULL,3);
}

void destroy_session_mgr() {
	destroy_timer_list(GATEWAY_TIMER_LIST);
}

struct session* get_server_session(int stype) {
	if (stype < 0 || stype > STYPE_MAX) {
		return NULL;
	}

	return SESSION_MGR.server_session[stype];
}

void gateway_schedule(void(*on_time)(void* data), void* kdata, int after_sec) {
	schedule_timer(GATEWAY_TIMER_LIST, on_time, kdata, after_sec);
}

void check_server_online(void* data) {
	//printf("check_server_online\n");
	if (0 == SESSION_MGR.need_connectd) {
		return;
	}
	SESSION_MGR.need_connectd = 0;
	//for (int i = 0; i < GW_CONFIG.num_server_moudle;++i) {
	for (int i = 0; i < 1; ++i) {
		int stype = GW_CONFIG.module_set[i].stype;
		if (NULL == SESSION_MGR.server_session[stype]) {
			SESSION_MGR.server_session[stype] = gateway_connect(GW_CONFIG.module_set[i].ip, GW_CONFIG.module_set[i].port, stype);
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
	if (stype < 0 || stype > STYPE_MAX) {
		return;
	}
	SESSION_MGR.server_session[stype] = NULL;
	SESSION_MGR.need_connectd = 1; //设置重新连接
}

