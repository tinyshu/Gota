#ifndef __TCP_SESSION_H__
#define __TCP_SESSION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../3rd/mjson/json.h"

#ifdef __cplusplus
}
#endif

#define MAX_SEND_PKG 2048

struct session {
	char c_ip[32];
	int c_port;
#ifdef USE_LIBUV
	void* c_sock;
#else
	int c_sock;
#endif
	
	int removed;
	//websocket是否握手成功
	int is_shake_hand;
	//用于区分前后端session协议类型
	int socket_type;
	unsigned int uid;
	int is_server_session;
	struct session* next;
	unsigned char send_buf[MAX_SEND_PKG];
};

void init_session_manager(int socket_type, int protocal_type);
void exit_session_manager();


// 有客服端进来，保存这个sesssion;
struct session* save_session(void* c_sock, char* ip, int port);
extern void close_session(struct session* s);

// 遍历我们session集合里面的所有session
void foreach_online_session(int(*callback)(struct session* s, void* p), void*p);

// 处理我们的数据
void clear_offline_session();
// end 

//发送数据接口
//应用层只用传入数据和长度即可，底层负责网络协议封包
extern void session_send(struct session* s, unsigned char* body, int len);

//发送json对象
extern void session_json_send(struct session* s, json_t* object);

extern int get_proto_type();

#endif

