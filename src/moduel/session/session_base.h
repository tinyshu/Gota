#ifndef SESSION_BASE_H__
#define SESSION_BASE_H__

struct recv_msg;

typedef struct session_base {
	session_base():is_server_session(0), utag(0){}
	virtual void close() = 0;
	virtual void send_data(unsigned char* pkg, int pkg_len) = 0;
	virtual void send_msg(recv_msg* msg) = 0;
public:
	int socket_type;
	//client连接server为0 server连接其他server为1
	int is_server_session;
	int utag;

}session_base;

#endif	