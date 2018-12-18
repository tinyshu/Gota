#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" {
#include "../../utils/hash_map_int.h"
}

#include "session_key_mgr.h"


struct {
	unsigned int key;
	struct hash_map_int* session_map;
}SESSINO_KEY_MGR;

void init_session_key_map() {
	memset(&SESSINO_KEY_MGR,0,sizeof(SESSINO_KEY_MGR));
	SESSINO_KEY_MGR.key = 0;
	SESSINO_KEY_MGR.session_map = create_hash_map_int();
	if (NULL == SESSINO_KEY_MGR.session_map) {
		exit(-1);
	}
}

void exit_session_key_map() {
	destory_hash_map(SESSINO_KEY_MGR.session_map);
}

unsigned int get_session_key() {
	unsigned int key = ++SESSINO_KEY_MGR.key;
	return key;
}

void save_session_by_key(unsigned int key, struct session* s) {
	set_hash_map_int(SESSINO_KEY_MGR.session_map, key,(void*)s);
}

void remove_session_by_key(unsigned int key) {
	remove_hash_int_by_key(SESSINO_KEY_MGR.session_map, key);
}

void clear_session_by_value(struct session* s) {
	if (NULL==s) {
		return;
	}
	remove_hash_int_by_value(SESSINO_KEY_MGR.session_map,(void*)s);
}

struct session* get_session_by_key(unsigned int key) {
	return (struct session*)get_value_by_key(SESSINO_KEY_MGR.session_map,key);
}
