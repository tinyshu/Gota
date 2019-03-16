#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef WIN32
#include <WinSock2.h>
//#include <Windows.h>
#endif

#include "tcp_session.h"
#include "../net/proto_type.h"
#include "../net/net_uv.h"
#include "../../3rd/mjson/json_extends.h"
#include "../netbus/netbus.h"
#include "../../moduel/netbus/recv_msg.h"
#include "../../proto/proto_manage.h"
#include "../../utils/mem_manger.h"
#include "../netbus/service_manger.h"

#define MAX_SESSION_NUM 6000
#define my_malloc malloc
#define my_free free

#define MAX_RECV_BUFFER 8096

//////////////////////////////////////////////////////////
//实现session_base接口
void session::close() {
	close_session(this);
}

void session::send_data(unsigned char* pkg, int pkg_len) {
	session_send(this, pkg, pkg_len);
}

void session::send_msg(recv_msg* msg) {
	int pkg_len = 0;
	unsigned char* pkg = protoManager::encode_cmd_msg(msg, &pkg_len);
	if (pkg == NULL || pkg_len == 0) {
		//log
		return;
	}
	session_send(this, pkg, pkg_len);
	if (pkg!=NULL) {
		memory_mgr::get_instance().free_memory(pkg);
		//my_free(pkg);
	}
	
}

void session::send_raw_msg(raw_cmd* raw_data) {
	if (raw_data==NULL) {
		return;
	}
	session_send(this, raw_data->raw_data, raw_data->raw_len);
}
///////////////////////////////////////////////////////
extern void on_connect_lost(struct session* s);

struct {
	struct session* online_session;

	struct session* cache_mem;
	struct session* free_list;
	int readed; // 当前已经从socket里面读取的数据;
	int has_removed;
	int socket_type;  //0 tcp_socket 1表示websocket
	int protocal_type;// 0 表示二进制协议，size + 数据的模式
	               // 1,表示文本协议，以回车换行来分解收到的数据为一个包
}session_manager;

static struct session* cache_alloc() {
	struct session* s = NULL;
	if (session_manager.free_list != NULL) {
		s = session_manager.free_list;
		session_manager.free_list = s->next;
	}
	else { // 调用系统的函数 malloc
		s = new (struct session);
	}
	
	return s;
}

static void cache_free(struct session* s) {
	// 判断一下，是从cache分配出去的，还是从系统my_malloc分配出去的？
	if (s >= session_manager.cache_mem && s < session_manager.cache_mem + MAX_SESSION_NUM) {
		s->next = session_manager.free_list;
		session_manager.free_list = s;
	}
	else { 
		delete s;
	}
	
}

//tcp or websocket
int get_socket_type() {
	return session_manager.socket_type;
}

//统一包头(8字节：2字节长度+2字节stype+2字节cmd+4字节utag ) +body(protobuf or json)
int get_proto_type() {
	return session_manager.protocal_type;
}

void init_socket_and_proto_type(int socket_type, int protocal_type) {
	session_manager.socket_type = socket_type;
	session_manager.protocal_type = protocal_type; //本次框架使用的协议类型
}

void init_session_manager() {
	memset(&session_manager, 0, sizeof(session_manager));
	//session_manager.socket_type = socket_type;
	//session_manager.protocal_type = protocal_type; //本次框架使用的协议类型
	// 将6000个session一次分配出来。
	session_manager.cache_mem = new struct session[MAX_SESSION_NUM];
	
	// end 

	for (int i = 0; i < MAX_SESSION_NUM; i++) {
		session_manager.cache_mem[i].next = session_manager.free_list;
		session_manager.free_list = &session_manager.cache_mem[i];
	}

}

void exit_session_manager() {

}

struct session* save_session(void* c_sock, char* ip, int port) {
	struct session* s = cache_alloc();
#ifdef USE_LIBUV
	s->c_sock = c_sock;
#else
	s->c_sock = (int)c_sock;
#endif
	
	s->c_sock = c_sock;
	s->c_port = port;
	int len = strlen(ip);
	if (len >= 32) {
		len = 31;
	}
	strncpy(s->c_ip, ip, len);
	s->c_ip[len] = 0;

	s->next = session_manager.online_session;
	session_manager.online_session = s;
	return s;
}

void foreach_online_session(int(*callback)(struct session* s, void* p), void*p) {
	if (callback == NULL) {
		return;
	}

	struct session* walk = session_manager.online_session;
	while (walk) {
		if (walk->removed == 1) {
			walk = walk->next;
			continue;
		}
		if (callback(walk, p)) {
			return;
		}
		walk = walk->next;
	}
}

void close_session(struct session* s) {
	s->removed = 1;
	session_manager.has_removed = 1;
	//通知lua层，调用on_session_disconnect
	server_manage::get_instance().on_session_disconnect(s);
	//printf("client %s:%d exit\n", s->c_ip, s->c_port);
}

void clear_offline_session() {
	if (session_manager.has_removed == 0) {
		return;
	}

	struct session** walk = &session_manager.online_session;
	while (*walk) {
		struct session* s = (*walk);
		if (s->removed) {
			*walk = s->next;
			s->next = NULL;
			//通知上层session关闭
			//on_connect_lost(s);
			/*if (s->c_sock != 0) {
				closesocket(s->c_sock);
			}*/

			s->c_sock = 0;
			// 释放session
			cache_free(s);
		}
		else {
			walk = &(*walk)->next;
		}
	}
	session_manager.has_removed = 0;
}

static int send_tcp_data(struct session* s, unsigned char* data, int len) {
	unsigned char* pkg_ptr = NULL;

	if (len + 2 > MAX_SEND_PKG) {
		pkg_ptr = (unsigned char*)memory_mgr::get_instance().alloc_memory(len + 2);
	}
	else {
		pkg_ptr = (unsigned char*)memory_mgr::get_instance().alloc_memory(MAX_SEND_PKG);
	}
	int ssize = 0;
	if (session_manager.protocal_type == JSON_PROTOCAL) {
		memcpy(pkg_ptr, data, len);
		strncpy((char*)pkg_ptr + len, "\r\n", 2);
		//ssize = send(s->c_sock, pkg_ptr, len + 2, 0);
		uv_send_data(s->c_sock, (char*)pkg_ptr, len + 2);
	}
	else if (session_manager.protocal_type == BIN_PROTOCAL) {
		memcpy(pkg_ptr + 2, data, len);
		pkg_ptr[0] = ((len + 2)) & 0x000000ff;
		pkg_ptr[1] = (((len + 2)) & 0x0000ff00) >> 8;
		uv_send_data(s->c_sock, (char*)pkg_ptr, len + 2);
	}
	if (pkg_ptr != NULL) {
		memory_mgr::get_instance().free_memory(pkg_ptr);
	}

	return (len+2);
}

//发送websocket数据
static int send_websocket_data(struct session* s, unsigned char* data, int len) {
	
	//int long_pkg = 0;
	unsigned char* pkg_ptr = NULL;

	int head_len = 1; //固定一字节
	if (len <= 125) {
		++head_len;
	}
	else if (len <= 0xffff) {
		head_len += 3; //126 + 长度占2个字节
	}
	else {
		head_len += 9; //127+8字节长度s
	}

	if (len + head_len > MAX_SEND_PKG) {
		//需要动态分配内存
		//pkg_ptr = (unsigned char*)my_malloc(len + head_len);
		pkg_ptr = (unsigned char*)memory_mgr::get_instance().alloc_memory(len + head_len);
	}
	else {
		//pkg_ptr = (unsigned char*)my_malloc(MAX_SEND_PKG);
		pkg_ptr = (unsigned char*)memory_mgr::get_instance().alloc_memory(MAX_SEND_PKG);
	}

	//发送格式 固定1字节0x81 + 数据长度(变长) + 数据
	//发送写入的位置
	unsigned int send_len = 0;
	//websocket协议第一个字节0x81或者0x82
	//这里约定websocket协议二进制头的json协议第一个字节为0x82
	//pkg_ptr[0] = 0x81;
	pkg_ptr[0] = 0x82;
	if (len <= 125) {
		pkg_ptr[1] = len;
		send_len = 2;
	}
	else if (len <= 0xffff) {
		pkg_ptr[1] = 126;
		pkg_ptr[2] = (len & 0x000000ff);
		pkg_ptr[3] = ((len & 0x0000ff00) >> 8);
		send_len = 4;
	}
	else {
		pkg_ptr[1] = 127;
		pkg_ptr[2] = (len & 0x000000ff);
		pkg_ptr[3] = ((len & 0x0000ff00) >> 8);
		pkg_ptr[4] = ((len & 0x00ff0000) >> 16);
		pkg_ptr[5] = ((len & 0xff000000) >> 24);
		//不使用，不会有这么长的包，前32个字节足够表示了
		pkg_ptr[6] = 0;
		pkg_ptr[7] = 0;
		pkg_ptr[8] = 0;
		pkg_ptr[9] = 0;
		send_len = 10;

	}
	//拷贝数据,并发送
# ifdef _DEBUG
	printf("发送数据给web:%s\n", data);
#endif

	//copy数据到缓存区
	memcpy(pkg_ptr + send_len, data, len);
	send_len += len; //整个数据包长度
	//int send_bytes = send(s->c_sock, pkg_ptr, send_len, 0);
	uv_send_data(s->c_sock, (char*)pkg_ptr, send_len);
	if (pkg_ptr!=NULL){
		//my_free(pkg_ptr);
		//pkg_ptr = NULL;
		memory_mgr::get_instance().free_memory(pkg_ptr);
	}

	return send_len;
}

void session_send(struct session* s, unsigned char* body, int len) {
	if (s == NULL || body == NULL || len <= 0) {
		return;
	}
	int send_bytes = 0;
	if (TCP_SOCKET_IO == s->socket_type) {
		send_bytes = send_tcp_data(s, body, len);
	}
	else if (WEB_SOCKET_IO == s->socket_type) {
		send_bytes = send_websocket_data(s, body, len);
	}
#ifdef _DEBUG
	printf("send_bytes%d\n", send_bytes);
#endif
}

void session_json_send(struct session* s, json_t* json_object) {
	if (NULL == s || NULL == json_object) {
		printf("session_json_send s is null\n");
		return;
	}
	char* json_str = NULL;
	json_tree_to_string(json_object,&json_str);
	session_send(s, (unsigned char*)json_str,strlen(json_str));
	json_free_str(json_str);
}