#ifndef MYSQL_WRAPPER_H__
#define MYSQL_WRAPPER_H__

#include <vector>
#include <map>
#include <string.h>
#include "query_callback.h"

class mysql_wrapper {
public:
	static void connect(const char* ip, int port, const char* db_name, const char* user_name,
		const char* passwd, cb_connect_db connect_db, void* udata = NULL);

	static void close(void* context, cb_close_db on_close);

	static void query(void* context,const char* sql, cb_query_db on_query);
	static void query2map(void* context, const char* sql, cb_query_db_res_map on_query_map);
	//不返回结果集
	static void query_no_res(void* context, const char* sql, cb_query_no_res_cb on_query_res_cb);
};

#endif