#ifndef RECV_MSG_H_
#define RECV_MSG_H_

#define BIN_HEAD_LEN (sizeof(msg_head))

//2字节标识后面一个完整的包长度
#define SIZE_HEAD 2
//不管是json还是pb格式通用头
typedef struct msg_head {
	//service编号
	int stype;
	//协议cmdid
	int ctype;
	//用户uid
	unsigned int utag;
}msg_head;

typedef struct recv_msg {
	
	msg_head head;
	/*
	JSON字符串或者是pb二进制格式;
	*/
	void* body; 
}recv_msg;

//用于网关服务解析包头，不解析body直接转发给其他service
typedef struct raw_cmd {
	msg_head head;
	//指向原始数据头(包含包头)
	unsigned char* raw_data;
	int raw_len;
}raw_cmd;
#endif
