#ifndef CENTER_DB_CONFIG_H__
#define CENTER_DB_CONFIG_H__

struct center_db_config {
	//mysql
	char* mysql_ip;
	char* mysql_name;
	int mysql_port;
	char* mysql_pwd;
	char* database_name;

	//redis
	//end 
};

extern struct center_db_config CENTER_DB_CONFIG;
#endif