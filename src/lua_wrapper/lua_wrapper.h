#ifndef LUA_WRAPPER_H__
#define LUA_WRAPPER_H__

#include "lua.hpp"
#define SERVICE_FUNCTION_MAPPING "service_function_map"

extern lua_State* g_lua_state;

class lua_wrapper {
public:
	static void init_lua();
	static void exit_lua();

	static int exce_lua_file(const char* lua_file_path);
	static void add_search_path(const char* path);
	//static lua_wrapper& get_instance();

	static void reg_func2lua(const char* func_name, int (*lua_function)(lua_State* L));
	static lua_State* get_luastatus();

	static const char* read_table_by_key(lua_State* L,const char* table_name,const char* key);
	//C++执行lua函数
	static int execute_lua_script_by_handle(int handle_id,int args_num);
	static int remove_lua_script_by_handle(int handle_id);
	//执行lua注册在表里的函数
	static void get_service_function_by_refid(lua_State* L, int refid);
	static int execute_service_fun_by_handle(int handle_id, int args_num);
	static int push_service_fun_by_handle(int handle_id);
	static int remove_service_fun_by_handle(int handle_id);
	static int push_function_by_handle(int handle_id);
private:
	
	static int execute_function(int args_num);
};

#endif