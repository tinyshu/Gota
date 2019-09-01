#include <stdlib.h>
#include <string.h>
#include "utils_export_to_lua.h"
#include "../utils/timestamp.h"
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


#define my_malloc malloc
#define my_free free

const char * utils_moduel_name = "utils_wrapper";

static int lua_timestamp(lua_State* tolua_s) {
	unsigned long ts = timestamp();
	lua_pushinteger(tolua_s, ts);
	return 1;
}

static int lua_timestamp_today(lua_State* tolua_s) {
	unsigned long ts = timestamp_today();
	lua_pushinteger(tolua_s, ts);
	return 1;
}

static int lua_timestamp_yesterday(lua_State* tolua_s) {
	unsigned long ts = timestamp_yesterday();
	lua_pushinteger(tolua_s, ts);
	return 1;
}


int register_utils_export_tolua(lua_State*tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, utils_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, utils_moduel_name);
		tolua_function(tolua_s, "timestamp", lua_timestamp);
		tolua_function(tolua_s, "timestamp_today", lua_timestamp_today);
		tolua_function(tolua_s, "timestamp_yesterday", lua_timestamp_yesterday);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}