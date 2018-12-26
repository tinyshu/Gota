#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "google\protobuf\message.h"
#include "service_export_to_lua.h"
#include "../utils/conver.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif

#include "../moduel/netbus/service_interface.h"
#include "../utils/logger.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "hiredis.h"
#include "../moduel/net/net_uv.h"
#include "../3rd/tolua/tolua_fix.h"
#include "redis_warpper.h"
#include "query_callback.h"
#include "../moduel/netbus/netbus.h"
#include "../moduel/netbus/service_manger.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/netbus/recv_msg.h"

const char * service_moduel_name = "service_wrapper";
static unsigned s_function_ref_id = 0;

using namespace google::protobuf;

//存储lua传入进来的函数handle_id
class lua_service_module :public service {
public:
	lua_service_module() {

		on_session_recv_cmd_handle = 0;
		on_session_disconnect_handle = 0;
	}

	virtual bool on_session_recv_cmd(struct session_base*s, recv_msg* msg);
	virtual void on_session_disconnect(struct session* s);
public:
	int on_session_recv_cmd_handle;
	int on_session_disconnect_handle;
};

static void push_pb_message_tolua(const Message* pb_msg) {
	if (pb_msg==NULL) {
		return;
	}

	lua_State* lua_status = lua_wrapper::get_luastatus();

	lua_newtable(lua_status);
	const Descriptor* descriptor = pb_msg->GetDescriptor();
	if (descriptor==NULL) {
		return;
	}

	const Reflection* refletion = pb_msg->GetReflection();
	if (refletion == NULL) {
		return;
	}

	for (int i = 0; i < descriptor->field_count();++i) {
		const FieldDescriptor* filedes = descriptor->field(i);
		
		const std::string& file_name = filedes->lowercase_name();
		lua_pushstring(lua_status, file_name.c_str());

		//是否为一个数组
		if (filedes->is_repeated()) {
			//遍历数组每个元素
			lua_newtable(lua_status);
			//数组大小
			int size = refletion->FieldSize(*pb_msg, filedes);
			for (int i = 0; i < size;++i) {
				//基本类型，或者是内嵌的message
				switch (filedes->cpp_type()) {
				case FieldDescriptor::CPPTYPE_DOUBLE: {
					lua_pushnumber(lua_status, refletion->GetDouble(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_FLOAT: {
					lua_pushnumber(lua_status, refletion->GetFloat(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_INT64: {
					std::string ss = convert<int64, std::string>(refletion->GetInt64(*pb_msg, filedes));
					lua_pushstring(lua_status, ss.c_str());

				}break;
				case FieldDescriptor::CPPTYPE_UINT64: {
					std::string ss = convert<int64, std::string>(refletion->GetUInt64(*pb_msg, filedes));
					lua_pushstring(lua_status, ss.c_str());
				}break;
				case FieldDescriptor::CPPTYPE_ENUM: {
					lua_pushinteger(lua_status, refletion->GetEnumValue(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_INT32: {
					lua_pushinteger(lua_status, refletion->GetInt32(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_UINT32: {
					lua_pushinteger(lua_status, refletion->GetUInt32(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_STRING: {
					std::string ss = refletion->GetString(*pb_msg, filedes);
					lua_pushstring(lua_status, ss.c_str());
				}break;
				case FieldDescriptor::CPPTYPE_BOOL: {
					lua_pushboolean(lua_status, refletion->GetBool(*pb_msg, filedes));
				}break;
				case FieldDescriptor::CPPTYPE_MESSAGE: {
					//又是内嵌的Message,需要递归在此调用push_pb_message_tolua
					push_pb_message_tolua(pb_msg);
				}break;
				default: {

				}break;
				}//switch
				
				//t[n] = v,v在栈顶 这个函数会将值弹出栈
				lua_rawseti(lua_status, -2, i + 1);
			}
		}
		else {
			//基本类型，或者是内嵌的message
			switch (filedes->cpp_type()) {
			case FieldDescriptor::CPPTYPE_DOUBLE: {
				lua_pushnumber(lua_status, refletion->GetDouble(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_FLOAT: {
				lua_pushnumber(lua_status, refletion->GetFloat(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_INT64: {
				std::string ss = convert<int64, std::string>(refletion->GetInt64(*pb_msg, filedes));
				lua_pushstring(lua_status, ss.c_str());
				
			}break;
			case FieldDescriptor::CPPTYPE_UINT64:{
				std::string ss = convert<int64, std::string>(refletion->GetUInt64(*pb_msg, filedes));
				lua_pushstring(lua_status, ss.c_str());
			}break;
			case FieldDescriptor::CPPTYPE_ENUM: {
				lua_pushinteger(lua_status, refletion->GetEnumValue(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_INT32: {
				lua_pushinteger(lua_status, refletion->GetInt32(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_UINT32: {
				lua_pushinteger(lua_status, refletion->GetUInt32(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_STRING: {
				std::string ss = refletion->GetString(*pb_msg, filedes);
				lua_pushstring(lua_status, ss.c_str());
			}break;
			case FieldDescriptor::CPPTYPE_BOOL: {
				lua_pushboolean(lua_status,refletion->GetBool(*pb_msg, filedes));
			}break;
			case FieldDescriptor::CPPTYPE_MESSAGE: {
				//又是内嵌的Message,需要递归在此调用push_pb_message_tolua
				push_pb_message_tolua(pb_msg);
			}break;
			default: {

			}break;
			}//switch
		}
		//t[k] = v  这个函数会将键和值都弹出栈
		lua_rawset(lua_status,-3);
	}//for

}

//当收到消息会根据stype来调用对应的函数,然后把协议数据放入栈，调用lua函数
//bool lua_service_module::on_session_recv_cmd(struct session_base* s, recv_msg* msg) {
bool lua_service_module::on_session_recv_cmd(struct session_base* s, recv_msg* msg) {
	
	if (s==NULL || msg==NULL) {
		return false;
	}
	lua_State* lua_status = lua_wrapper::get_luastatus();
	int idx = 1;
	//C++网络底层使用struct session
	tolua_pushuserdata(lua_status,s->get_lua_session());

	//创建一个表，存入{1: stype, 2 ctype, 3 utag, 4 body str}
	lua_newtable(lua_status);

	lua_pushinteger(lua_status, msg->stype);
	lua_rawseti(lua_status,-2,idx);
	idx++;

	lua_pushinteger(lua_status, msg->ctype);
	lua_rawseti(lua_status, -2, idx);
	idx++;

	lua_pushinteger(lua_status, msg->utag);
	lua_rawseti(lua_status, -2, idx);
	idx++;

	if (msg->body == NULL) {
		lua_pushnil(lua_status);
	}
	else {
		if (get_proto_type() == BIN_PROTOCAL) {
			//二进制是pb格式
			//stack : msg {1: stype, 2 ctype, 3 utag, 4 body table_or_str}
			//pb的数据转成table格式,这个时候就需要Message对象的name,value信息
			push_pb_message_tolua((const Message*)msg->body);
		}
		else if (get_proto_type() == JSON_PROTOCAL) {
			//字符串json格式
			//stack : msg {1: stype, 2 ctype, 3 utag, 4 body table_or_str}
			//json到lua层，直接把json文本放入堆栈
			lua_pushstring(lua_status,(char*)msg->body);
		}
		lua_rawseti(lua_status, -2, idx);
		idx++;
	}

	//执行lua回调函数
	if (lua_wrapper::execute_service_fun_by_handle(on_session_recv_cmd_handle, 2)==0) {
		lua_wrapper::remove_service_fun_by_handle(on_session_recv_cmd_handle);
	}
	
	return true;
}

void lua_service_module::on_session_disconnect(struct session* s) {
	tolua_pushuserdata(lua_wrapper::get_luastatus(),s);
	if (lua_wrapper::execute_service_fun_by_handle(on_session_disconnect_handle, 1) == 0) {
		lua_wrapper::remove_service_fun_by_handle(on_session_disconnect_handle);
	}
}

static void init_service_function_map(lua_State* L) {
	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	lua_newtable(L);
	//作一个等价于 t[k] = v 的操作， 这里 t 是一个给定有效索引 index 处的值， v 指栈顶的值， 而 k 是栈顶之下的那个值
	//这个函数会把键和值都从堆栈中弹出
	lua_rawset(L, LUA_REGISTRYINDEX);
}

//lo函数地址在栈中的位置
static unsigned int save_service_function(lua_State* L, int lo, int def)
{
	if (!lua_isfunction(L, lo)){
		return 0;
	}

	s_function_ref_id++;

	lua_pushstring(L, SERVICE_FUNCTION_MAPPING);
	//把 t[k] 值压入堆栈， 这里的 t 是指有效索引 index 指向的值， 而 k 则是栈顶放的值。
	//这个函数会弹出堆栈上的 key （把结果放在栈上相同位置）
	//通过伪索引获取SERVICE_FUNCTION_MAPPING表，并放入栈顶
	lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: fun ... refid_fun */
	//放入函数对应的func_id
	lua_pushinteger(L, s_function_ref_id);                      /* stack: fun ... refid_fun refid */
	//现在栈最上面是 func_id table table_name
	//把堆栈上给定有效处索引处的元素作一个拷贝压栈
	lua_pushvalue(L, lo);                                       /* stack: fun ... refid_fun refid fun */

	/*
	t[k] = v 的操作
	这里 t 是一个给定有效索引 index 处的值， v 指栈顶的值， 而 k 是栈顶之下的那个值
	函数会把键和值都从堆栈中弹出
	*/
	lua_rawset(L, -3);                  /* refid_fun[refid] = fun, stack: fun ... refid_ptr */
	lua_pop(L, 1);                                              /* stack: fun ... */

	return s_function_ref_id;
}

/*lua注册service模块 
local my_service = {
-- msg {1: stype, 2 ctype, 3 utag, 4 body_table_or_str}
on_session_recv_cmd = function(session, msg)

end,

on_session_disconnect = function(session)
end
}

local ret = service.register(100, my_service)
*/
int register_service(lua_State* tolua_s) {
	int stype = (int)tolua_tonumber(tolua_s, 1,0);
	if (stype <= 0 || stype > MAX_SERVICES) {
		return -1;
	}

	if (tolua_istable(tolua_s,2,0,NULL)==0) {
		return -1;
	}

	//获取lua传入的table的值，就是函数地址 3-4
	lua_getfield(tolua_s, 2, "on_session_recv_cmd");
	lua_getfield(tolua_s, 2, "on_session_disconnect");
	
	int on_session_recv_cmd_handle = save_service_function(tolua_s, 3, 0);
	int on_session_disconnect_handle = save_service_function(tolua_s, 4, 0);
	
	if (on_session_recv_cmd_handle ==0 && on_session_disconnect_handle ==0) {
		return -1;
	}

	lua_service_module* lua_module = new lua_service_module;
	if (lua_module == NULL) {
		return -1;
	}
	lua_module->stype = stype;
	lua_module->on_session_recv_cmd_handle = on_session_recv_cmd_handle;
	lua_module->on_session_disconnect_handle = on_session_disconnect_handle;
	//注册到service管理模块
	server_manage::get_instance().register_service(stype, lua_module);
	return 0;
}

int register_service_export_tolua(lua_State*tolua_s) {
	init_service_function_map(tolua_s);
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, service_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, service_moduel_name);
		tolua_function(tolua_s, "register_service", register_service);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);

	return 0;
}