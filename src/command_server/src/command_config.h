#ifndef __COMMAND_CONFIG_H__
#define __COMMAND_CONFIG_H__

struct command_config {
	char* ip;
	int port;

	// Êý¾Ý¿â£¬redis
	// end 
};

extern struct command_config COMMAND_CONF;
#endif

