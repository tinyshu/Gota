#ifndef MYSQL_EXPORT_TO_LUA_H__
#define MYSQL_EXPORT_TO_LUA_H__

struct lua_State;
int register_mysql_export_tolua(lua_State*L);

#endif