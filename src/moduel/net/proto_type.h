#ifndef PROTO_TYPE_H__
#define PROTO_TYPE_H__
//底层传输协议
enum {
	TCP_SOCKET_IO = 0,  // tcp
	WEB_SOCKET_IO = 1,  // websocket
};

//应用层协议
enum {
	BIN_PROTOCAL = 0, // 二进制协议 pb
	JSON_PROTOCAL = 1, // json协议
};

#endif