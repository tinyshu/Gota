#ifndef PROTO_MANAGE_H__
#define PROTO_MANAGE_H__

#include <string.h>
#include <unordered_map>
struct recv_msg;

class proroManager {
public:
	static void register_cmd(char** cmd_array,int cmd_count);
	static void register_cmd(int cmd_id, char* cmd);
	static bool decode_cmd_msg(unsigned char* pkg, int pkg_len, recv_msg** out_msg);
	static unsigned char* encode_cmd_msg(recv_msg* msg,int * out_len);
	static const std::string get_cmmand_protoname(int cmd);
	static void msg_free(recv_msg* msg);
private:
	static std::unordered_map<int, std::string> _cmd_map;
	
};
#endif