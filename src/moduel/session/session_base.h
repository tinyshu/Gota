#ifndef SESSION_BASE_H__
#define SESSION_BASE_H__

struct recv_msg;

typedef struct session_base {
	virtual void close() = 0;
	virtual void send_data(unsigned char* pkg, int pkg_len) = 0;
	virtual void send_msg(recv_msg* msg) = 0;

}session_base;

#endif	