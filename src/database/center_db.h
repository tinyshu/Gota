#ifndef CONNECT_CENTER_DB_H__
#define CONNECT_CENTER_DB_H__

#ifdef WIN32
#include <winsock.h>
//#include <windows.h>
#include "mysql.h"
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")
#pragma comment(lib, "libmysql.lib")
#endif

struct user_info {
	unsigned int uid;
	char unick[64];
	int uface;
	int usex;
	int is_guest;
	char phone_num[20];
};


extern MYSQL* mysql_center;

void connect_to_centerdb();

int get_userinfo_buy_key(const char* rand_key,struct user_info* userinfo);

int insert_guest_with_key(char* ukey, char* unick,int uface, int usex);
#endif