#ifndef LUA_WRAPPER_H__
#define LUA_WRAPPER_H__

#include "lua.hpp"

class lua_wrapper {
public:
	static void init_lua();
	static void exit_lua();

	static int exce_lua_file(const char* lua_file_path);

	static lua_wrapper& get_instance();

	static void reg_func2lua(const char* func_name, int (*lua_function)(lua_State* L));
private:
	
};

#endif