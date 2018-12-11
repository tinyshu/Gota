#ifndef LUA_WRAPPER_H__
#define LUA_WRAPPER_H__

#include "lua.hpp"
extern lua_State* g_lua_state;

class lua_wrapper {
public:
	static void init_lua();
	static void exit_lua();

	static int exce_lua_file(const char* lua_file_path);

	static lua_wrapper& get_instance();

	static void reg_func2lua(const char* func_name, int (*lua_function)(lua_State* L));
	static lua_State* get_luastatus();

	//C++Ö´ÐÐluaº¯Êý
	static int execute_lua_script_by_handle(int handle_id,int args_num);
	static int remove_lua_script_by_handle(int handle_id);
private:
	static int push_function_by_handle(int handle_id);
	static int execute_function(int args_num);
};

#endif