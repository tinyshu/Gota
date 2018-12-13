#ifndef REDIS_WRAPPER_H__
#define REDIS_WRAPPER_H__

#include <vector>
#include <map>
#include <string.h>
#include "query_callback.h"

class redis_wrapper {
public:
	static void rediseconnect(const char* ip, int port, int timeout, cb_connect_db connect_db, void* udata = NULL);

	static void rediseclose(void* context, cb_close_db on_close);

	static void redisequery(void* context, const char* sql, redis_query_cb f_query_cb);

};


#endif