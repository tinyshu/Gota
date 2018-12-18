#ifndef RECV_MSG_H_
#define RECV_MSG_H_

typedef struct recv_msg {
	int stype;
	int ctype;
	unsigned int utag;
	void* body; // JSON str ªÚ’ﬂ «message;
}recv_msg;

#endif
