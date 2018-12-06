#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lua_wrapper.h"
#include "../../utils/logger.h"


lua_State* g_lua_state = NULL;
static void do_log_message(void(*log)(const char* file_name, int line_num, const char* msg), 	const char* msg) {	lua_Debug info;	int depth = 0;	while (lua_getstack(g_lua_state, depth, &info)) {		lua_getinfo(g_lua_state, "S", &info);		lua_getinfo(g_lua_state, "n", &info);		lua_getinfo(g_lua_state, "l", &info);		if (info.source[0] == '@') {			log(&info.source[1], info.currentline, msg);			return;		}		++depth;	}	if (depth == 0) {		log("trunk", 0, msg);	}}
//获取lua的调用信息
static void print_error(const char* filename,int line_num,const char* msg) {
	logger::log(filename, line_num,ERROR,msg);
}

static void print_waring(const char* filename, int line_num, const char* msg) {
	logger::log(filename, line_num, WARNING, msg);
}

static void print_debug(const char* filename, int line_num, const char* msg) {
	logger::log(filename, line_num, DEBUG, msg);
}


//lua只能导出这样签名的C函数
int lua_logdebug(lua_State* L) {
	//去栈顶string类型的元素
	const char* msg = luaL_checkstring(L,-1);
	if (msg!=NULL) {
		do_log_message(print_debug, msg);
	}
	return 0;
}

int lua_logwarning(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg != NULL) {
		do_log_message(print_waring,msg);
	}
	return 0;
}

int lua_logerror(lua_State* L) {
	const char* msg = luaL_checkstring(L, -1);
	if (msg != NULL) {
		do_log_message(print_error, msg);
	}
	return 0;
}

lua_wrapper& lua_wrapper::get_instance() {
	static lua_wrapper _instance;
	return _instance;
}

static intlua_panic(lua_State* pState) {	const char *msg = lua_tostring(pState, -1);	do_log_message(print_error, msg);	return 0;}

void lua_wrapper::init_lua() {
	g_lua_state = luaL_newstate();
	//挂载luapanic函数，如果不挂接自定义函数	//当lua一次，会调用abort函数终止进程	lua_atpanic(g_lua_state, lua_panic);
	//注册lua库 
	luaL_openlibs(g_lua_state);
	//导出框架接口
	reg_func2lua("LOGDEBUG", lua_logdebug);
	reg_func2lua("LOGWARNING", lua_logwarning);
	reg_func2lua("LOGERROR", lua_logerror);
}

void lua_wrapper::exit_lua() {
	if (g_lua_state!=NULL) {
		lua_close(g_lua_state);
		g_lua_state = NULL;
	}
}

int lua_wrapper::exce_lua_file(const char* lua_file_path) {
	if (0!=luaL_dofile(g_lua_state,lua_file_path)) {
		lua_logerror(g_lua_state);
		return -1;
	}
	return 0;
}

void lua_wrapper::reg_func2lua(const char* func_name, int(*cfunction)(lua_State* L)) {
	lua_pushcfunction(g_lua_state, cfunction);
	lua_setglobal(g_lua_state,func_name);
}


