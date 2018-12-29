#include <stdlib.h>
#include <string.h>
#include <sstream>
#include "google\protobuf\message.h"
#include "session_export_to_lua.h"
#include "../utils/conver.h"
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
#include "../moduel/netbus/netbus.h"
#include "../moduel/netbus/service_manger.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/netbus/recv_msg.h"
#include "../proto/proto_manage.h"

const char * session_moduel_name = "session_wrapper";
static unsigned s_function_ref_id = 0;
using namespace google::protobuf;
using namespace std;

//导出到lua层的都是export_session的接口实例
int lua_close_session(lua_State* tolua_s) {

	session_base* s = (session_base*)lua_touserdata(tolua_s,1);
	if (s==NULL) {
		return 0;
	}
	s->close();
	return 0;
}

static Message* create_message_from_lua_table(lua_State* tolua_s,int table_idx,const string& type_name) {
	if (type_name.empty() || !lua_istable(tolua_s,table_idx)) {
		return NULL;
	}

	/*
	通过名称创建对应的Message,返回父类指针，实际创建的
	是和名字对应的类实例
	*/
	Message* message = proroManager::create_message_by_name(type_name);
	if (message==NULL) {
		return NULL;
	}

	const Reflection* reflection = message->GetReflection();
	const Descriptor* descriptor = message->GetDescriptor();

	//通过lua传入的table来赋值Message，如果对应的字段是required类型，
	//并且没有获取值，返回失败
	int file_count = descriptor->field_count();
	for (int i = 0; i < file_count;++i) {
		const FieldDescriptor* filedes = descriptor->field(i);
		const string& file_name = filedes->name();
		if (file_name.empty()) {
			return NULL;
		}

		bool is_required = filedes->is_required();
		bool is_repeated = filedes->is_repeated();
		
		//把字段数据放到栈顶
		lua_pushstring(tolua_s, file_name.c_str());
		//table[栈顶strnig]的值放入栈顶,并弹出string
		lua_rawget(tolua_s, table_idx);

		//栈顶元素是否有值
		bool isNil = lua_isnil(tolua_s, -1);
		if (is_repeated) {
			//数组类型
			if (isNil) {
				lua_pop(tolua_s, 1);
				continue;
			}

			if (!lua_istable(tolua_s,-1)) {
				log_error("cant find repeated field %s\n", file_name.c_str());
				delete message;
				message = NULL;
				return NULL;
			}
			else {
				lua_pushnil(tolua_s);
				//lua_next会先弹出栈顶元素，所以要先放一个nil
				//然后把key,value放入 key放在-2 value放在-1的位置
				while (lua_next(tolua_s, -1) != 0) {
					switch (filedes->cpp_type()) {
					case FieldDescriptor::CPPTYPE_DOUBLE:
					{
						double value = luaL_checknumber(tolua_s, -1);
						reflection->SetDouble(message, filedes, value);

					}break;
					case FieldDescriptor::CPPTYPE_FLOAT:
					{
						float value = luaL_checknumber(tolua_s, -1);
						reflection->SetFloat(message, filedes, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_INT64:
					{
						int64_t value = luaL_checknumber(tolua_s, -1);
						reflection->SetInt64(message, filedes, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_UINT64:
					{
						uint64_t value = luaL_checknumber(tolua_s, -1);
						reflection->SetUInt64(message, filedes, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
					{
						int32_t value = luaL_checknumber(tolua_s, -1);
						const EnumDescriptor* enumDescriptor = filedes->enum_type();
						const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
						reflection->SetEnum(message, filedes, valueDescriptor);
					}
					break;
					case FieldDescriptor::CPPTYPE_INT32:
					{
						int32_t value = luaL_checknumber(tolua_s, -1);
						reflection->SetInt32(message, filedes, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_UINT32:
					{
						uint32_t value = luaL_checknumber(tolua_s, -1);
						reflection->SetUInt32(message, filedes, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_STRING:
					{
						size_t size = 0;
						const char* value = luaL_checklstring(tolua_s, -1, &size);
						reflection->SetString(message, filedes, std::string(value, size));
					}
					break;
					case FieldDescriptor::CPPTYPE_BOOL:
					{
						bool value = lua_toboolean(tolua_s, -1);
						reflection->SetBool(message, filedes, value);
					}
					break;
					case FieldDescriptor::CPPTYPE_MESSAGE:
					{
						//递归调用
						Message* value = create_message_from_lua_table(tolua_s, lua_gettop(tolua_s), filedes->message_type()->name().c_str());
						if (!value) {
							log_error("convert to message %s failed whith value %s \n", filedes->message_type()->name().c_str(), file_name.c_str());
							delete message;
							message = NULL;
							return NULL;
						}
						Message* msg = reflection->MutableMessage(message, filedes);
						msg->CopyFrom(*value);
						delete msg;
						msg = NULL;
					}
					break;
					default:
						break;
					} //switch

					lua_pop(tolua_s, 1);
				}
			}
		}
		else {
			//基础类型或者嵌套类型
			if (is_required) {
				if (isNil) {
					log_error("cant find required field %s\n", file_name.c_str());
					delete message;
					message = NULL;
					return NULL;
				}
			}
			else {
				if (isNil) {
					lua_pop(tolua_s, 1);
					continue;
				}
			}
			switch (filedes->cpp_type()) {
			case FieldDescriptor::CPPTYPE_DOUBLE:
			{
				double value = luaL_checknumber(tolua_s, -1);
				reflection->SetDouble(message, filedes, value);
				
			}break;
			case FieldDescriptor::CPPTYPE_FLOAT:
			{
				float value = luaL_checknumber(tolua_s, -1);
				reflection->SetFloat(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_INT64:
			{
				int64_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetInt64(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_UINT64:
			{
				uint64_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetUInt64(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
			{
				int32_t value = luaL_checknumber(tolua_s, -1);
				const EnumDescriptor* enumDescriptor = filedes->enum_type();
				const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
				reflection->SetEnum(message, filedes, valueDescriptor);
			}
			break;
			case FieldDescriptor::CPPTYPE_INT32:
			{
				int32_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetInt32(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_UINT32:
			{
				uint32_t value = luaL_checknumber(tolua_s, -1);
				reflection->SetUInt32(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_STRING:
			{
				size_t size = 0;
				const char* value = luaL_checklstring(tolua_s, -1, &size);
				reflection->SetString(message, filedes, std::string(value, size));
			}
			break;
			case FieldDescriptor::CPPTYPE_BOOL:
			{
				bool value = lua_toboolean(tolua_s, -1);
				reflection->SetBool(message, filedes, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_MESSAGE:
			{
				//递归调用
				Message* value = create_message_from_lua_table(tolua_s, lua_gettop(tolua_s), filedes->message_type()->name().c_str());
				if (!value) {
					log_error("convert to message %s failed whith value %s \n", filedes->message_type()->name().c_str(), file_name.c_str());
					delete message;
					message = NULL;
					return NULL;
				}
				Message* msg = reflection->MutableMessage(message, filedes);
				msg->CopyFrom(*value);
				delete msg;
				msg = NULL;
			}
			break;
			default:
				break;
			} //switch

		}
	}
	return message;
}

// {1: stype, 2: ctype, 3: utag, 4 body}
int lua_send_msg(lua_State* tolua_s) {
	session_base* s = (session_base*)lua_touserdata(tolua_s, 1);
	if (s == NULL) {
		return 0;
	}

	//获取表value值,并放入栈上3-6的位置
	lua_getfield(tolua_s, 2,"stype");
	lua_getfield(tolua_s, 2, "ctype");
	lua_getfield(tolua_s, 2, "utag");
	lua_getfield(tolua_s, 2, "body");

	struct recv_msg msg;
	msg.stype = (int)lua_tointeger(tolua_s,3);
	msg.ctype = (int)lua_tointeger(tolua_s, 4);
	msg.utag = (int)lua_tointeger(tolua_s, 5);
	if (get_proto_type() == JSON_PROTOCAL) {
		//json数据之君子而获取string然后发送
		msg.body = (void*)lua_tostring(tolua_s,6);
		s->send_msg(&msg);
	}
	else if(get_proto_type() == BIN_PROTOCAL){
		//pb二进制数据,lua需要{}的格式发送数据，在C++层在转换成Message对象
		if (!lua_istable(tolua_s,6)) {
			//没有数据
			msg.body = NULL;
			s->send_msg(&msg);
		}
		else {
			string type_name = proroManager::get_cmmand_protoname(msg.ctype);
			if (type_name.empty()) {
				msg.body = NULL;
				s->send_msg(&msg);
			}

			Message* pb_msg = create_message_from_lua_table(tolua_s,6,type_name);
			if (pb_msg==NULL) {
				//error log
				log_error("create_message_from_lua_table field error %s\n", type_name.c_str());
				return 0;
			}

			msg.body = (void*)pb_msg;
			s->send_msg(&msg);
			delete pb_msg;
			pb_msg = NULL;
		}
	}

	return 0;
}


int register_session_export_tolua(lua_State*tolua_s) {
	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, session_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, session_moduel_name);
		tolua_function(tolua_s, "close_session", lua_close_session);
		tolua_function(tolua_s, "send_msg", lua_send_msg);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}