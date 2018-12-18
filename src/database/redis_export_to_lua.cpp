#include <stdlib.h>
#include <string.h>

#include "mysql_export_to_lua.h"
#include "../utils/logger.h"
#include "hiredis.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"

#ifdef __cplusplus
}
#endif
#include "../moduel/net/net_uv.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "../3rd/tolua/tolua_fix.h"
#include "redis_warpper.h"
#include "query_callback.h"

const char * redist_moduel_name = "redis_wrapper";

//redis连接回调，在通知给lua的回调函数
//context是lock_redis对象
//udata是lua function的id
static void connect_redis_cb(char* error, void* context, void* udata) {
	if (error!=NULL) {
		lua_pushstring(lua_wrapper::get_luastatus(), error);
		lua_pushnil(lua_wrapper::get_luastatus());
	}
	else {
		lua_pushnil(lua_wrapper::get_luastatus());
		tolua_pushuserdata(lua_wrapper::get_luastatus(), context);
	}

	int handle_id = (int)udata;
	if (lua_wrapper::execute_lua_script_by_handle(handle_id, 2) == 0) {
		lua_wrapper::remove_lua_script_by_handle(handle_id);
	}
	
}


int lua_redis_connect(lua_State* tolua_s) {
	
	char* redis_ip = (char*)tolua_tostring(tolua_s, 1, NULL);
	if (redis_ip==NULL) {
		return -1;
	}

	int redis_port = (int)tolua_tonumber(tolua_s, 2, 0);
	if (redis_port<=0) {
		return -1;
	}

	int redis_time = (int)tolua_tonumber(tolua_s, 3, 0);

	//获取lua的回调函数handle
	int s_function_ref_id = toluafix_ref_function(tolua_s, 4, 0);
	
	redis_wrapper::rediseconnect(redis_ip, redis_port, redis_time,connect_redis_cb,(void*)s_function_ref_id);
	return 0;
}

int lua_redis_close(lua_State* tolua_s) {
	void* context = (void*)lua_touserdata(lua_wrapper::get_luastatus(),-1);
	if (context!=NULL) {
		redis_wrapper::rediseclose(context,NULL);
	}
	return 0;
}

static void push_redis_reply(redisReply* reply) {

	switch (reply->type) {
		case REDIS_REPLY_STRING:
		case REDIS_REPLY_STATUS:{
			lua_pushstring(lua_wrapper::get_luastatus(),reply->str);
		}break;
		case REDIS_REPLY_INTEGER: {
			lua_pushinteger(lua_wrapper::get_luastatus(), reply->integer);
		}break;
		case REDIS_REPLY_ERROR: {
		}break;
		case REDIS_REPLY_ARRAY: {
			//转成{}放入栈中
			lua_newtable(lua_wrapper::get_luastatus());
			int idx = 1;
			for (int i = 0; i < reply->elements;++i) {
				push_redis_reply(reply->element[i]);
				lua_rawseti(lua_wrapper::get_luastatus(),-2,idx);
				idx++;
			}
		}break;
	}
}
static void query_redis_cb(char* err,redisReply* reply,void* udata) {
	if (err!=NULL || reply==NULL) {
		lua_pushstring(lua_wrapper::get_luastatus(), err);
		lua_pushnil(lua_wrapper::get_luastatus());
	}
	else {
		lua_pushnil(lua_wrapper::get_luastatus());
		if (reply!=NULL) {
			//redis返回的数据要根据type来处理
			push_redis_reply(reply);
		}
		else {
			lua_pushnil(lua_wrapper::get_luastatus());
		}
	}

	lua_wrapper::execute_lua_script_by_handle((int)udata, 2);
	lua_wrapper::remove_lua_script_by_handle((int)udata);
}

int lua_redis_query(lua_State* tolua_s) {
	void* context = (void*)lua_touserdata(lua_wrapper::get_luastatus(), 1);
	if (context==NULL) {
		return -1;
	}
	
	char* cmd = (char*)tolua_tostring(lua_wrapper::get_luastatus(), 2,0);
	if (cmd==NULL) {
		return -1;
	}

	int s_function_ref_id = toluafix_ref_function(lua_wrapper::get_luastatus(), 3,0);
	if (s_function_ref_id<=0) {
		return -1;
	}

	redis_wrapper::redisequery(context,cmd, query_redis_cb,(void*)s_function_ref_id);
	return 0;
}

int register_redis_export_tolua(lua_State* tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, redist_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, redist_moduel_name);
		tolua_function(tolua_s, "connect", lua_redis_connect);
		tolua_function(tolua_s, "close", lua_redis_close);
		tolua_function(tolua_s, "query", lua_redis_query);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);

	return 0;
}