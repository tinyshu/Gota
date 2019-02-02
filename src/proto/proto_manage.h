#ifndef PROTO_MANAGE_H__
#define PROTO_MANAGE_H__

#include "google/protobuf/message.h"
#include <string.h>
#include <unordered_map>
struct recv_msg;
struct raw_cmd;

class protoManager {
public:
	static void register_cmd(char** cmd_array,int cmd_count);
	static void register_cmd(int cmd_id, const char* cmd);
	static bool decode_cmd_msg(unsigned char* pkg, int pkg_len, recv_msg** out_msg);
	static unsigned char* encode_cmd_msg(recv_msg* msg,int * out_len);
	static const std::string get_cmmand_protoname(int cmd);
	static void msg_free(recv_msg* msg);
	static google::protobuf::Message* create_message_by_name(const std::string& type_name);
	static bool decode_rwa_cmd_msg(unsigned char* pkg, int pkg_len, raw_cmd* raw_out_msg);
private:
	static std::unordered_map<int, std::string> _cmd_map;
	
};
#endif