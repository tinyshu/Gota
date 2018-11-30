#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"
#include "types_service.h"

void json_get_uid_and_key(json_t* cmd, unsigned int* uid, unsigned int* s_key) {
	
	*uid = json_object_get_unsigned_number(cmd, "uid");
	*s_key = json_object_get_unsigned_number(cmd, "skey");
}

json_t*
json_new_server_return_cmd(int stype, int cmd,
	unsigned int uid,
	unsigned int s_key) {
	json_t* json = json_new_comand(stype + TYPE_OFFSET, cmd);

	json_object_push_number(json, "s_key", s_key);
	json_object_push_number(json, "uid", uid);
	return json;
}

void write_error(struct session* s, int stype,
	int cmd, int status,
	unsigned int uid, unsigned int s_key) {
	json_t* json = json_new_server_return_cmd(stype, cmd, uid, s_key);
	json_object_push_number(json, "2", status);
	session_json_send(s, json);
	json_free_value(&json);
}
