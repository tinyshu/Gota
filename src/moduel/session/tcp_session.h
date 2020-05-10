#ifndef __TCP_SESSION_H__
#define __TCP_SESSION_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "../../3rd/mjson/json.h"

#ifdef __cplusplus
}
#endif
#include <string.h>
#include "session_base.h"

#define MAX_SEND_PKG 2048
class export_session;
struct raw_cmd;

struct session:public session_base {

	session() {
		memset(c_ip,0,sizeof(c_ip));
		c_port = 0;
		c_sock = 0;
		removed = 0;
		is_shake_hand = 0;
		socket_type = 0;
		uid = 0;
		is_server_session = 0;
		next = NULL;
		memset(send_buf,0,sizeof(send_buf));
	}
	char c_ip[32];
	int c_port;
#ifdef USE_LIBUV
	void* c_sock;
#else
	int c_sock;
#endif
	
	int removed;
	//websocket�Ƿ����ֳɹ�
	int is_shake_hand;
	//��������ǰ���sessionЭ������
	
	unsigned int uid;
	//int is_server_session;
	struct session* next;
	unsigned char send_buf[MAX_SEND_PKG];

public:
	virtual void close();
	virtual void send_data(unsigned char* pkg, int pkg_len);
	virtual void send_msg(recv_msg* msg);
	virtual void send_raw_msg(raw_cmd* raw_data);
	virtual const char* get_address(int* client_port);
};

void init_session_manager();
void exit_session_manager();


// �пͷ��˽������������sesssion;
struct session* save_session(void* c_sock, char* ip, int port);
extern void close_session(struct session* s);

// ��������session�������������session
void foreach_online_session(int(*callback)(struct session* s, void* p), void*p);

// �������ǵ�����
void clear_offline_session();
// end 

//�������ݽӿ�
//Ӧ�ò�ֻ�ô������ݺͳ��ȼ��ɣ��ײ㸺������Э����
extern void session_send(struct session* s, unsigned char* body, int len);

//����json����
extern void session_json_send(struct session* s, json_t* object);

extern int get_proto_type();
extern int get_socket_type();
void init_socket_and_proto_type(int socket_type, int protocal_type);
#endif

