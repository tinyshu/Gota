#ifndef SESSION_BASE_H__
#define SESSION_BASE_H__

struct recv_msg;
struct raw_cmd;
/*
session模块
最开始用纯C实现，所以用的struct,
后面改为C++，就继续struct把
*/
typedef struct session_base {
	session_base():is_server_session(0), utag(0), _uid(0){}
	virtual void close() = 0;
	virtual void send_data(unsigned char* pkg, int pkg_len) = 0;
	virtual void send_msg(recv_msg* msg) = 0;
	virtual void send_raw_msg(raw_cmd* raw_data) = 0;
	virtual const char* get_address(int* client_port) = 0;

	void set_utag(int u_tag) { utag = u_tag; }
	int  get_utag() { return utag; }

	void set_is_server_session(int session_type) { is_server_session = session_type; }
	int  get_is_server_session() { return is_server_session; }

	void set_uid(int uid) { _uid = uid; }
	int get_uid() { return _uid; }
public:
	int socket_type;
	//client连接server为0 server连接其他server为1
	int is_server_session;
	int utag;
	int _uid; 

}session_base;

#endif	