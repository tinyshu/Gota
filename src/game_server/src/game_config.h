#ifndef __GAME_CONFIG_H__
#define __GAME_CONFIG_H__

struct game_config {
	char* ip;
	int port;

	// Êý¾Ý¿â£¬redis
	// end 
};

extern struct game_config GAME_CONF;
#endif

