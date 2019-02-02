#ifndef __UDP_SESSION_H__
#define __UDP_SESSION_H__
#include "session_base.h"

#define MAX_SEND_PKG 2048

extern uv_loop_t* get_uv_loop();

class export_session;
class export_udp_session;
//udp使用一个缓存区接受数据
typedef struct udp_recv_buf {
	unsigned char* recv_buf; //malloc申请的空间
	size_t max_recv_len; //缓存区大小
}udp_recv_buf;

struct udp_session : public session_base {
	udp_session():sock_addr(NULL){}
public:
	virtual void close();
	virtual void send_data(unsigned char* pkg, int pkg_len);
	virtual void send_msg(recv_msg* msg);
	virtual void send_raw_msg(raw_cmd* raw_data);
public:
	static void start_udp_server(const char* ip, int port);
	static udp_recv_buf _recv_buf;
	uv_udp_t * udp_handle;
	char address[32];
	int port;
	const sockaddr_in* sock_addr;
};


#endif