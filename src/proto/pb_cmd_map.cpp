#include <stdio.h>
#include <string.h>

#include "pb_cmd_map.h"
#include "proto_manage.h"
char* g_pb_cmd_map[] = {
	"LoginReq", 
	"LoginRes",
};

void init_pb_cmd_map() {
	protoManager::register_cmd(g_pb_cmd_map,2);
}