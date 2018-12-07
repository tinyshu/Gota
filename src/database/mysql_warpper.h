#ifndef MYSQL_WRAPPER_H__
#define MYSQL_WRAPPER_H__

#include <vector>
#include <string.h>

typedef std::vector<std::vector<std::string> > DBRES;
typedef void(*cb_connect_db)(char* error, void* context);
typedef void(*cb_query_db)(char*error, DBRES* res);
class mysql_wrapper {
public:
	static void connect(const char* ip, int port, const char* db_name, const char* user_name,
		const char* passwd, cb_connect_db connect_db);

	static void close(void* context);

	static void query(void* context,const char* sql, cb_query_db on_query);
};

#endif