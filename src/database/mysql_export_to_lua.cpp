#include <stdlib.h>
#include <string.h>

#include "mysql_export_to_lua.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif
#include "../lua_wrapper/lua_wrapper.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../database/mysql_warpper.h"
#include "mysql.h"

void on_connect_db_cb(char* error, void* context, void* udata) {
	if (error!=NULL) {
		lua_pushstring(lua_wrapper::get_luastatus(),error);
		lua_pushnil(lua_wrapper::get_luastatus());
		
	}
	else {
		lua_pushnil(lua_wrapper::get_luastatus());
		//放入指针到lua栈
		tolua_pushuserdata(lua_wrapper::get_luastatus(), context);
	}

	lua_wrapper::execute_lua_script_by_handle((int)udata,2);
	lua_wrapper::remove_lua_script_by_handle((int)udata);
}

int lua_mysql_connect(lua_State* tolua_s) {
	char* ip = (char*)tolua_tostring(tolua_s,1,NULL);
	int port = (int)tolua_tonumber(tolua_s,2,0);
	char* db_name = (char*)tolua_tostring(tolua_s,3,NULL);
	char* user_name = (char*)tolua_tostring(tolua_s, 4, NULL);
	char* passwd = (char*)tolua_tostring(tolua_s, 5, NULL);

	int s_function_ref_id = toluafix_ref_function(tolua_s, 6, NULL);
	mysql_wrapper::connect(ip, port, db_name, user_name, passwd, on_connect_db_cb,(void*)s_function_ref_id);
	return 0;
}

int lua_mysql_close(lua_State* tolua_s) {
	void* context = (void*)lua_touserdata(tolua_s, -1);
	if (context) {
		mysql_wrapper::close(context, NULL);
	}
	return 0;
}

static void query_push_row(MYSQL_ROW row,int filed_num) {
	lua_newtable(lua_wrapper::get_luastatus());
	int idx = 1;
	for (int i = 0; i < filed_num;++i) {
		if (row[i]==NULL) {
			lua_pushnil(lua_wrapper::get_luastatus());
		}
		else {
			lua_pushstring(lua_wrapper::get_luastatus(), row[i]);
		}
		lua_rawseti(lua_wrapper::get_luastatus(),-2,idx);
		idx++;
	}
}

static void query_db_cb(char*error, MYSQL_RES* res,void* udata) {
	if (error!=NULL) {
		lua_pushstring(lua_wrapper::get_luastatus(),error);
		lua_pushnil(lua_wrapper::get_luastatus());
	}
	else {
		//把MYSQL_RES成lua的 二维表形式{{},{}}形式
		if (res != NULL) {
			lua_pushnil(lua_wrapper::get_luastatus()); //error
			//栈顶创建一个{}
			lua_newtable(lua_wrapper::get_luastatus());

			int field_num = mysql_num_fields(res);
			int idx = 1; //存入lua{}的位置,从1开始
			MYSQL_ROW row;
			while ((row = mysql_fetch_row(res)) != NULL) {
				/*
				lua_rawseti (lua_State *L, int idx, lua_Integer n)
				函数解释:把栈顶元素赋值给idx指向对象的n位置,相当于 table[n] = 栈顶元素
				*/
				//1.先要把mysql一行数据放入到{}，作为栈顶元素
				query_push_row(row, field_num);
				//2.使用lua_rawseti给table赋值
				lua_rawseti(lua_wrapper::get_luastatus(),-2,idx);
				idx++;
			}
		}
		else {
			//push2个nil放入栈顶，lua层读取结果集是nil
			//说明没有查询到数据
			lua_pushnil(lua_wrapper::get_luastatus()); //error
			lua_pushnil(lua_wrapper::get_luastatus()); //res
		}
	}

	lua_wrapper::execute_lua_script_by_handle((int)udata, 2);
	lua_wrapper::remove_lua_script_by_handle((int)udata);
}


int lua_mysql_query(lua_State* tolua_s) {
	void* context = (void*)lua_touserdata(tolua_s, 1);
	if (context==NULL) {
		return -1;
	}

	char* sql = (char*)tolua_tostring(tolua_s, 2, 0);
	if (sql == NULL) {
		return -1;
	}

	int s_function_ref_id = toluafix_ref_function(tolua_s, 3, NULL);
	if (s_function_ref_id<=0) {
		return -1;
	}
	mysql_wrapper::query(context,sql, query_db_cb,(void*)s_function_ref_id);
	return 0;
}
int register_mysql_export_tolua(lua_State* tolua_s) {
	//_G是lua中全局table
	lua_getglobal(tolua_s,"_G");
	if (lua_istable(tolua_s,-1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s,"mysql_wrapper",0);
		
		//开始导出模块接口
		tolua_beginmodule(tolua_s,"mysql_wrapper");
		tolua_function(tolua_s,"connect", lua_mysql_connect);
		tolua_function(tolua_s,"close", lua_mysql_close);
		tolua_function(tolua_s, "query", lua_mysql_query);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}
