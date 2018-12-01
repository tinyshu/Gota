#ifndef __CENTER_CONFIG_H__
#define __CENTER_CONFIG_H__

struct center_config {
	char* ip;
	int port;

	//mysql
	char* mysql_ip;
	char* mysql_name;
	int mysql_port;
	char* mysql_pwd;
	char* database_name;

	//redis
	//end 
};

extern struct center_config CENTER_CONF;
#endif

