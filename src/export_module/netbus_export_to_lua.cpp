#include <stdlib.h>
#include <string.h>
#include "netbus_export_to_lua.h"
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
#include "../moduel/netbus/netbus.h"

#define my_malloc malloc
#define my_free free

const char *  netbus_moduel_name = "netbus_wrapper";

static int lua_tcp_listen(lua_State* tolua_s) {
	//返回栈顶元素的索引。 因为索引是从 1 开始编号的， 所以这个结果等于堆栈上的元素个数
	int argc = lua_gettop(tolua_s);
	if (argc!=2) {
		return 0;
	}

	char* ip = (char*)lua_tostring(tolua_s,1);
	int port = lua_tonumber(tolua_s,2);
	tcp_listen(ip, port);
	return 0;
}

static int lua_udp_listen(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 2) {
		return 0;
	}

	char* ip = (char*)lua_tostring(tolua_s, 1);
	int port = lua_tonumber(tolua_s, 2);
	udp_listen(ip, port);
	return 0;
}

static void on_connect_cb(const char* err, session_base* s, void* udata) {
	lua_State* L = lua_wrapper::get_luastatus();
	if (err!=NULL) {
		lua_pushstring(L, err);
		lua_pushnil(L);
	}
	else {
		lua_pushnil(L);
		tolua_pushuserdata(L, s);
	}

	lua_wrapper::execute_lua_script_by_handle((int)udata, 2);
	lua_wrapper::remove_lua_script_by_handle((int)udata);
}
//ip,port ,func
static int lua_tcp_connect(lua_State* tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 3) {
		return 0;
	}

	const char* ip = lua_tostring(tolua_s,1);
	if(ip==NULL){
		return 0;
	}

	int port = lua_tonumber(tolua_s,2);
	int func_handle = toluafix_ref_function(tolua_s,3,0);
	if (func_handle==0) {
		return 0;
	}

	tcp_connection(ip,port, on_connect_cb,(void*)func_handle);
}
int register_betbus_export_tolua(lua_State*tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, netbus_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, netbus_moduel_name);
		tolua_function(tolua_s, "tcp_listen", lua_tcp_listen);
		tolua_function(tolua_s, "udp_listen", lua_udp_listen);
		tolua_function(tolua_s, "tcp_connect", lua_tcp_connect);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}
