
#include <stdlib.h>
#include <string.h>
//
#include "proto_mgr_export_to_lua.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "../3rd/lua/lua.h"
#include "../3rd/tolua/tolua++.h"
#ifdef __cplusplus
}
#endif
#include "../moduel/net/proto_type.h"
#include <google/protobuf/message.h>
#include "../utils/logger.h"
#include "../lua_wrapper/lua_wrapper.h"
#include "../3rd/tolua/tolua_fix.h"
#include "../proto/proto_manage.h"
#include "../../moduel/netbus/recv_msg.h"
#include "../utils/mem_manger.h"
#include "../moduel/session/tcp_session.h"
//#include "../moduel/net/net_uv.h"


const char *  proto_moduel_name = "proto_mgr_wrapper";
#define my_malloc malloc
#define my_free free

static int lua_register_protobuf_cmd(lua_State*tolua_s) {
	/*
	lua注册协议传入一个table字符串
	返回给定索引处值的“长度”
	*/
	int array_len = luaL_len(tolua_s,1);
	for (int i= 1; i <= array_len;++i) {
		lua_pushnumber(tolua_s,i);
		lua_gettable(tolua_s,1);
		const char* cmd_name = luaL_checkstring(tolua_s,-1);
		if (cmd_name!=NULL) {
			protoManager::register_cmd(i, cmd_name);
		}
		lua_pop(tolua_s, 1);
	}
	return 0;
}

//proto_mgr_wrapper.read_msg_head
static int lua_read_msg_head(lua_State*tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc!=1) {
		return 0;
	}
	raw_cmd* raw_data = (raw_cmd*)lua_touserdata(tolua_s,-1);
	if (raw_data==NULL) {
		lua_pushinteger(tolua_s, 0);
		lua_pushinteger(tolua_s, 0);
		lua_pushinteger(tolua_s, 0);
		return 3;
	}

	lua_pushinteger(tolua_s, raw_data->head.stype);
	lua_pushinteger(tolua_s, raw_data->head.ctype);
	lua_pushinteger(tolua_s, raw_data->head.utag);
	return 3;
}


void push_proto_message_tolua(const google::protobuf::Message* message) {
	lua_State* state = lua_wrapper::get_luastatus();
	if (!message) {
		return;
	}
	const google::protobuf::Reflection* reflection = message->GetReflection();
	

	// 顶层table
	lua_newtable(state);

	const google::protobuf::Descriptor* descriptor = message->GetDescriptor();
	for (int32_t index = 0; index < descriptor->field_count(); ++index) {
		const google::protobuf::FieldDescriptor* fd = descriptor->field(index);
		const std::string& name = fd->lowercase_name();
		
		
		// key
		lua_pushstring(state, name.c_str());

		bool bReapeted = fd->is_repeated();

		if (bReapeted) {
			// repeated这层的table
			lua_newtable(state);
			int size = reflection->FieldSize(*message, fd);
			for (int i = 0; i < size; ++i) {
				char str[32] = { 0 };
				switch (fd->cpp_type()) {
				case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
					lua_pushnumber(state, reflection->GetRepeatedDouble(*message, fd, i));
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
					lua_pushnumber(state, (double)reflection->GetRepeatedFloat(*message, fd, i));
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
					sprintf(str, "%lld", (long long)reflection->GetRepeatedInt64(*message, fd, i));
					lua_pushstring(state, str);
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:

					sprintf(str, "%llu", (unsigned long long)reflection->GetRepeatedUInt64(*message, fd, i));
					lua_pushstring(state, str);
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
					lua_pushinteger(state, reflection->GetRepeatedEnum(*message, fd, i)->number());
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
					lua_pushinteger(state, reflection->GetRepeatedInt32(*message, fd, i));
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
					lua_pushinteger(state, reflection->GetRepeatedUInt32(*message, fd, i));
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
				{
					std::string value = reflection->GetRepeatedString(*message, fd, i);
					lua_pushlstring(state, value.c_str(), value.size());
				}
				break;
				case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
					lua_pushboolean(state, reflection->GetRepeatedBool(*message, fd, i));
					break;
				case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
					push_proto_message_tolua(&(reflection->GetRepeatedMessage(*message, fd, i)));
					break;
				default:
					break;
				}

				lua_rawseti(state, -2, i + 1); // lua's index start at 1
			}

		}
		else {
			char str[32] = { 0 };
			switch (fd->cpp_type()) {

			case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
				lua_pushnumber(state, reflection->GetDouble(*message, fd));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
				lua_pushnumber(state, (double)reflection->GetFloat(*message, fd));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_INT64:

				sprintf(str, "%lld", (long long)reflection->GetInt64(*message, fd));
				lua_pushstring(state, str);
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:

				sprintf(str, "%llu", (unsigned long long)reflection->GetUInt64(*message, fd));
				lua_pushstring(state, str);
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
				lua_pushinteger(state, (int)reflection->GetEnum(*message, fd)->number());
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
				lua_pushinteger(state, reflection->GetInt32(*message, fd));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
				lua_pushinteger(state, reflection->GetUInt32(*message, fd));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
			{
				std::string value = reflection->GetString(*message, fd);
				lua_pushlstring(state, value.c_str(), value.size());
			}
			break;
			case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
				lua_pushboolean(state, reflection->GetBool(*message, fd));
				break;
			case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
				push_proto_message_tolua(&(reflection->GetMessage(*message, fd)));
				break;
			default:
				break; 
			}
		}
		//相当于table[key] = value
		lua_rawset(state, -3);
	}
}

static int lua_msg_read_body(lua_State*tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 1) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}

	raw_cmd* raw_data = (raw_cmd*)lua_touserdata(tolua_s, -1);
	if (raw_data == NULL) {
		lua_pushinteger(tolua_s, 0);
		return 1;
	}
	recv_msg* msg;
	if (!protoManager::decode_cmd_msg(raw_data->raw_data, raw_data->raw_len, &msg)) {
		memory_mgr::get_instance().free_memory(msg);
		lua_pushinteger(tolua_s, 0);
		return 1;
	}
	if (msg->body==NULL) {
		lua_pushnil(tolua_s);
	}else if (get_proto_type() == BIN_PROTOCAL) {
		push_proto_message_tolua((google::protobuf::Message*)msg->body);
	}else if (get_proto_type() == JSON_PROTOCAL) {
		lua_pushstring(tolua_s,(const char*)msg->body);
	}

	if (msg!=NULL) {
		if (get_proto_type() == BIN_PROTOCAL) {
			if (msg->body!=NULL) {
				delete (google::protobuf::Message*)msg->body;
			}
		}else if (get_proto_type() == JSON_PROTOCAL) {
			memory_mgr::get_instance().free_memory(msg->body);
		}
		memory_mgr::get_instance().free_memory(msg);
	}
	return 1;
}

static int lua_set_utag(lua_State*tolua_s) {
	int argc = lua_gettop(tolua_s);
	if (argc != 2) {
		return 0;
	}
	raw_cmd* raw_data = (raw_cmd*)lua_touserdata(tolua_s, 1);
	if (raw_data == NULL) {
		return 0;
	}

	int utag = luaL_checkinteger(tolua_s,2);
	unsigned char* utag_ptr = raw_data->raw_data + sizeof(int); //指向utag内存
	utag_ptr[0] = (utag & 0x000000FF);
	utag_ptr[1] = ((utag & 0x0000FF00) >> 8);
	utag_ptr[2] = ((utag & 0x00FF0000) >> 16);
	utag_ptr[3] = ((utag & 0xFF000000) >> 24);

	return 0;
}

int register_proto_export_tolua(lua_State*tolua_s) {

	lua_getglobal(tolua_s, "_G");
	if (lua_istable(tolua_s, -1)) {
		tolua_open(tolua_s);
		//注册一个导出模块
		tolua_module(tolua_s, proto_moduel_name, 0);

		//开始导出模块接口
		tolua_beginmodule(tolua_s, proto_moduel_name);
		tolua_function(tolua_s, "register_protobuf_cmd", lua_register_protobuf_cmd);
		tolua_function(tolua_s, "read_msg_head", lua_read_msg_head);
		//tolua_function(tolua_s, "read_json_msg_head", lua_read_json_msg_head);
		tolua_function(tolua_s, "set_raw_utag", lua_set_utag);
		tolua_function(tolua_s, "read_msg_body", lua_msg_read_body);
		tolua_endmodule(tolua_s);
	}
	lua_pop(tolua_s, 1);
	return 0;
}