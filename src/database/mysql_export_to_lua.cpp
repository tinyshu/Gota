#include <stdlib.h>
#include <string.h>

#include "mysql_export_to_lua.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif
#include "../lua_wrapper/lua_wrapper.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../database/mysql_warpper.h"

void on_connect_db_cb(char* error, void* context, void* udata) {
	if (error!=NULL) {
		lua_pushstring(lua_wrapper::get_luastatus(),error);
		lua_pushnil(lua_wrapper::get_luastatus());
		
	}
	else {
		lua_pushnil(lua_wrapper::get_luastatus());
		//放入指针到lua栈
		tolua_pushuserdata(lua_wrapper::get_luastatus(), context);
	}

	lua_wrapper::execute_lua_script_by_handle((int)udata,2);
	lua_wrapper::remove_lua_script_by_handle((int)udata);
}

int lua_mysql_connect(lua_State* tolua_s) {
	printf("lua_mysql_connect");
	char* ip = (char*)tolua_tostring(tolua_s,1,NULL);
	int port = tolua_tonumber(tolua_s,2,0);
	char* db_name = (char*)tolua_tostring(tolua_s,3,NULL);
	char* user_name = (char*)tolua_tostring(tolua_s, 4, NULL);
	char* passwd = (char*)tolua_tostring(tolua_s, 5, NULL);

	int s_function_ref_id = toluafix_ref_function(tolua_s, 6, NULL);
	mysql_wrapper::connect(ip, port, db_name, user_name, passwd, on_connect_db_cb,(void*)s_function_ref_id);
	return 0;
}

int lua_mysql_close(lua_State* tolua_s) {
	return 0;
}

int lua_mysql_query(lua_State* tolua_s) {
	return 0;
}
int register_mysql_export_tolua(lua_State* tolua_s) {
	//_G是lua中全局table
	lua_getglobal(tolua_s,"_G");
	if (lua_istable(tolua_s,-1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s,"mysql_wrapper",0);
		
		//开始导出模块接口
		tolua_beginmodule(tolua_s,"mysql_wrapper");
		tolua_function(tolua_s,"connect", lua_mysql_connect);
		tolua_function(tolua_s,"close", lua_mysql_close);
		tolua_function(tolua_s, "query", lua_mysql_query);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}
