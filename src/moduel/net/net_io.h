#ifndef NETIO_H__
#define NETIO_H__

//底层传输协议
enum {
	TCP_SOCKET_IO = 0,  // tcp
	WEB_SOCKET_IO = 1,  // websocket
};

//应用层协议
enum {
	BIN_PROTOCAL = 0, // 二进制协议
	JSON_PROTOCAL = 1, // json协议
};

const char* conver_socket_type_str(int socket_type);
const char* conver_protocal_str(int protocal_type);

void start_server(char* ip,int port ,int socket_type ,int proto_type);

void uv_send_data(void* stream, char* pkg, unsigned int pkg_len);
#endif

