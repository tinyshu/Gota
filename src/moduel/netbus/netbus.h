#ifndef NETBUS_H__
#define NETBUS_H__

#include "../../3rd/mjson/json.h"
extern struct timer_list* NETBUS_TIMER_LIST;
#define MAX_SERVICES 512
struct session_base;

struct service_module  {
	int stype;
	//注册
	void (*init_service_module)(struct service_module* module);
	//处理二进制协议接口
	int (*on_bin_protocal_data)(void* moduel_data,struct session* s,unsigned char* pkg,int len);

	//json协议处理接口
	int (*on_json_protocal_data)(void* moduel_data, struct session* s,json_t* root, unsigned char* pkg, int len);

	void (*on_connect_lost)(void* module_data, struct session* s);
	//关闭接口
	void* moduel_data;
};


void exit_server_netbus();
void init_server_netbus();
void on_bin_protocal_recv_entry(struct session_base* s, unsigned char* data, int len);
void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);
void register_services(int stype, struct service_module* module);

class netbus {
public:
	netbus() {}
	virtual ~netbus() {}

	static netbus & get_instance();

	void udp_listen(char* ip, int port);
	void udp_send_msg(const char* ip,int port, unsigned char* data, int len);
private:
	void* udp_handle;
};
void tcp_listen(char* ip,int port);
//void udp_listen(char* ip, int port);
void tcp_connection(const char* server_ip, int port, void(*connect_cb)(const char* err, session_base* s, void* udata), void* udata);
void run_loop();
void ws_listen(char* ip, int port);
#endif