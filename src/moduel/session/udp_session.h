#ifndef __UDP_SESSION_H__
#define __UDP_SESSION_H__
#include "session_base.h"

#define MAX_SEND_PKG 2048
class export_session;
class export_udp_session;
//udp使用一个缓存区接受数据
typedef struct udp_recv_buf {
	unsigned char* recv_buf; //malloc申请的空间
	size_t max_recv_len; //缓存区大小
}udp_recv_buf;

struct udp_session : public session_base {
	
	static void start_udp_server();
	
	virtual export_session* get_lua_session();
	
	export_udp_session* _udp_session;

	static udp_recv_buf _recv_buf;
};


#endif