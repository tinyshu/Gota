#ifndef AUTH_H__
#define AUTH_H__

#include "../../../3rd/mjson/json.h"

void guest_login(void* moduel_data, struct session* s,
	json_t* root, unsigned int uid, unsigned int skey);

#endif