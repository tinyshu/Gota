#include <stdlib.h>
#include <string.h>
#include "proto_mgr_export_to_lua.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif

#include "../utils/logger.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../proto/proto_manage.h"
#include "../../moduel/netbus/recv_msg.h"

#define my_malloc malloc
#define my_free free

const char *  proto_moduel_name = "proto_mgr_wrapper";

static int lua_register_protobuf_cmd(lua_State*tolua_s) {
	/*
	lua注册协议传入一个table字符串
	返回给定索引处值的“长度”
	*/
	int array_len = luaL_len(tolua_s,1);
	for (int i= 1; i <= array_len;++i) {
		lua_pushnumber(tolua_s,i);
		lua_gettable(tolua_s,1);
		const char* cmd_name = luaL_checkstring(tolua_s,-1);
		if (cmd_name!=NULL) {
			protoManager::register_cmd(i, cmd_name);
		}
		lua_pop(tolua_s, 1);
	}
	return 0;
}

//proto_mgr_wrapper.read_msg_head
static int lua_read_msg_head(lua_State*tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc!=1) {
		return 0;
	}
	raw_cmd* raw_data = (raw_cmd*)lua_touserdata(tolua_s,-1);
	if (raw_data==NULL) {
		return 0;
	}

	lua_pushinteger(tolua_s, raw_data->head.stype);
	lua_pushinteger(tolua_s, raw_data->head.ctype);
	lua_pushinteger(tolua_s, raw_data->head.utag);
	return 3;
}

static int lua_set_utag(lua_State*tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 2) {
		return 0;
	}
	raw_cmd* raw_data = (raw_cmd*)lua_touserdata(tolua_s, 1);
	if (raw_data == NULL) {
		return 0;
	}

	int utag = luaL_checkinteger(tolua_s,2);
	unsigned char* utag_ptr = raw_data->raw_data + 4; //指向utag内存
	utag_ptr[0] = (utag & 0x000000FF);
	utag_ptr[1] = ((utag & 0x0000FF00) >> 8);
	utag_ptr[2] = ((utag & 0x00FF0000) >> 16);
	utag_ptr[3] = ((utag & 0xFF000000) >> 24);

	return 0;
}

int register_proto_export_tolua(lua_State*tolua_s) {

	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, proto_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, proto_moduel_name);
		tolua_function(tolua_s, "register_protobuf_cmd", lua_register_protobuf_cmd);
		tolua_function(tolua_s, "read_msg_head", lua_read_msg_head);
		tolua_function(tolua_s, "set_utag", lua_set_utag);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}