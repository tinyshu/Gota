#ifndef SESSION_BASE_H__
#define SESSION_BASE_H__

class export_session;
class export_tcp_session;
class export_udp_session;

typedef struct session_base {
	virtual export_session* get_lua_session() = 0;
}session_base;

#endif	