#include <stdlib.h>
#include <string.h>
#include "timer_export_to_lua.h"
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
#include "../moduel/net/net_uv.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../utils/timer_uv_list.h"

#define my_malloc malloc
#define my_free free

typedef struct timer_handle {
	int ref; //lua传入的函数id
	int repeat_count;
}timer_handle;

const char * timer_moduel_name = "timer_wrapper";

static void on_lua_time(void* udata) {
	timer_handle* handle = (timer_handle*)udata;
	lua_wrapper::execute_lua_script_by_handle(handle->ref, 0);
	if (handle->repeat_count > 0) {
		handle->repeat_count--;
		if (handle->repeat_count==0) {
			lua_wrapper::remove_lua_script_by_handle(handle->ref);
		}
	}
}

int lua_create_timer(lua_State* tolua_s) {
	//定时器回调函数
	int s_function_ref_id = toluafix_ref_function(tolua_s, 1, NULL);
	if (s_function_ref_id <= 0) {
		toluafix_remove_function_by_refid(tolua_s, s_function_ref_id);
		lua_pushnil(tolua_s);
		return 1;
	}
	//重复执行次数，-1为无限次
	int repeat = tolua_tonumber(tolua_s, 2, 0);
	if (repeat < -1) {
		repeat = -1;
	}
    //定时器被创建多少时间后开始执行定时器回调函数
	int first_time_out = tolua_tonumber(tolua_s, 3, 0);
	//定时器执行间隔
	int interval = tolua_tonumber(tolua_s, 4, 0);

	timer_handle* handle = (timer_handle*)my_malloc(sizeof(timer_handle));
	if (handle==NULL) {
		toluafix_remove_function_by_refid(tolua_s, s_function_ref_id);
		lua_pushnil(tolua_s);
		return 1;
	}
	//记录lua层定时器回调函数handle
	handle->ref = s_function_ref_id;
	handle->repeat_count = repeat;
	uv_timer* timer = create_timer(on_lua_time, handle, repeat, first_time_out, interval);
	if (timer==NULL) {
		toluafix_remove_function_by_refid(tolua_s, s_function_ref_id);
		lua_pushnil(tolua_s);
		return 1;
	}

	tolua_pushuserdata(tolua_s, timer);
	return 1;
}

int lua_create_timer_once(lua_State* tolua_s) {
	int s_function_ref_id = toluafix_ref_function(tolua_s, 1, NULL);
	if (s_function_ref_id <= 0) {
		toluafix_remove_function_by_refid(tolua_s, s_function_ref_id);
		lua_pushnil(tolua_s);
		return 1;
	}
	
	int first_time_out = tolua_tonumber(tolua_s, 2, 0);
	int interval = tolua_tonumber(tolua_s, 3, 0);

	timer_handle* handle = (timer_handle*)my_malloc(sizeof(timer_handle));
	handle->ref = s_function_ref_id;
	handle->repeat_count = 1;
	uv_timer* timer = create_timer(on_lua_time, handle, 1, first_time_out, interval);
	if (timer == NULL) {
		lua_pushnil(tolua_s);
		toluafix_remove_function_by_refid(tolua_s,s_function_ref_id);
		return 1;
	}

	tolua_pushuserdata(tolua_s, timer);
	return 1;
}

int lua_cancle_timer(lua_State* tolua_s) {
	uv_timer* timer = (uv_timer*)tolua_touserdata(tolua_s,-1,NULL);
	if (timer==NULL) {
		return 0;
	}
	timer_handle* handle = (timer_handle*)get_timer_udata(timer);
	lua_wrapper::remove_lua_script_by_handle(handle->ref);
	my_free(handle);
	cancle_timer(timer);
	return 0;
}

int register_timer_export_tolua(lua_State*tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, timer_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, timer_moduel_name);
		tolua_function(tolua_s, "create_timer", lua_create_timer);
		tolua_function(tolua_s, "create_timer_once", lua_create_timer_once);
		tolua_function(tolua_s, "cancle_timer", lua_cancle_timer);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}
