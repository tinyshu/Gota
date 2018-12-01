#ifndef CONNECT_CENTER_DB_H__
#define CONNECT_CENTER_DB_H__

#ifdef WIN32
#include <winsock.h>
#include <windows.h>
#include "mysql.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")
#pragma comment(lib, "libmysql.lib")
#endif

extern MYSQL* mysql_center;

void connect_to_centerdb();
#endif