#ifndef SERVER_SESSION_H__
#define SERVER_SESSION_H__

#include "../../utils/timer.h"
#include "../../utils/timer_list.h"
#include "../../gate_moduel/session/tcp_session.h"
#include "../../types_service.h"

extern struct timer_list* NETBUS_TIMER_LIST;

void init_server_session();

struct session* get_server_session(int stype);

void check_server_online(void*);

void netbus_schedule(void(*on_time)(void* data), void* kdata, int after_sec);

void lost_server_connection(int stype);

void destroy_session_mgr();
#endif