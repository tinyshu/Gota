#ifndef UTILS_EXPORT_TO_LUA_H__
#define UTILS_EXPORT_TO_LUA_H__

//导出底层公共utils模块到lua层
struct lua_State;
int register_utils_export_tolua(lua_State*tolua_s);

#endif
