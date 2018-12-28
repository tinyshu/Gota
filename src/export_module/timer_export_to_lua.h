#ifndef TIMER_EXPORT_TO_LUA_H__
#define TIMER_EXPORT_TO_LUA_H__

struct lua_State;
//负责注册redis模块到lua虚拟机
int register_timer_export_tolua(lua_State*tolua_s);

#endif