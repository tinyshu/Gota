#ifndef __GW_CONFIG_H__
#define __GW_CONFIG_H__

struct server_module_config {
	int stype; 
	char* ip; // IP
	int port; // ¶Ë¿Ú
	char* desic;
};

struct gw_config {
	char* ip;
	int port;

	int num_server_moudle;
	struct server_module_config* module_set;
};

extern struct gw_config GW_CONFIG;

#endif

