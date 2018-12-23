#ifndef RECV_MSG_H_
#define RECV_MSG_H_

#define BIN_HEAD_LEN 8 
//2字节标识后面一个完整的包长度
#define SIZE_HEAD 2
typedef struct recv_msg {
	int stype;
	int ctype;
	unsigned int utag;
	void* body; // JSON str 或者是message;
}recv_msg;

#endif
