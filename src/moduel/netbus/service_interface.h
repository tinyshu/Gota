#ifndef __SERVICE_INTERFACE_H__
#define __SERVICE_INTERFACE_H__

struct session;

class service {
public:
	virtual bool on_session_recv_cmd(struct session* s, unsigned char* data, int len) = 0;
	virtual void on_session_disconnect(struct session* s) = 0;
};

#endif
