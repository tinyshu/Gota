#include "proto_manage.h"
#include "../moduel/netbus/recv_msg.h"
#include "../moduel/session/tcp_session.h"
#include "../moduel/net/net_uv.h"
#include "../utils/mem_manger.h"

using namespace google::protobuf;

std::unordered_map<int, std::string> protoManager::_cmd_map;

void protoManager::register_cmd(int cmd_id, const char* cmd) {
	_cmd_map[cmd_id] = std::string(cmd);
}

void protoManager::register_cmd(char** cmd_array, int cmd_count) {
	if (cmd_count <=0) {
		return;
	}

	for (int i = 0; i < cmd_count;++i) {
		_cmd_map[i]= std::string(cmd_array[i]);
	}
}

const std::string protoManager::get_cmmand_protoname(int cmd) {
	std::unordered_map<int, std::string>::iterator it = _cmd_map.find(cmd);
	if (it!= _cmd_map.end()) {
		return it->second;
	}

	return "";
}

//使用pb的反射机制创建对应的message对象 14
Message* protoManager::create_message_by_name(const std::string& type_name) {
	if (type_name.empty()) {
		return NULL;
	}
	//获取descriptor，每一个message都有一个定义的 descriptor
	//descriptor定义了message的元信息
	const Descriptor* descriptor  = DescriptorPool::generated_pool()->FindMessageTypeByName(type_name);
	if (descriptor == NULL) {
		return NULL;
	}

	const Message* prototype = MessageFactory::generated_factory()->GetPrototype(descriptor);
	if (prototype==NULL) {
		return NULL;
	}
	Message* message = prototype->New();
	return message;
}
//解析到完成二进制包调用
bool protoManager::decode_cmd_msg(unsigned char* pkg, int pkg_len, struct recv_msg** out_msg) {
	if (pkg==NULL || pkg_len<=0) {
		return false;
	}

	*out_msg = NULL;
	if (pkg_len < BIN_HEAD_LEN) {
		return false;
	}

	//先解码包头
	//recv_msg* msg = (recv_msg*)malloc(sizeof(recv_msg));
	recv_msg* msg = (recv_msg*)memory_mgr::get_instance().alloc_memory(sizeof(recv_msg));
	if (msg==NULL) {
		return false;
	}
	memset(msg,0,sizeof(recv_msg));
	*out_msg = msg;
	msg->head.stype = pkg[0] + (pkg[1] << 8);
	msg->head.ctype = pkg[2] + (pkg[3] << 8);
	msg->head.utag = pkg[4] + (pkg[5] << 8) + (pkg[6] << 16) + (pkg[7] << 24);

	if (pkg_len==BIN_HEAD_LEN) {
		
		return true;
	}
	const std::string type_name = protoManager::get_cmmand_protoname(msg->head.ctype);
	if (type_name.empty()) {
		//free(msg);
		memory_mgr::get_instance().free_memory(msg);
		msg = NULL;
		return false;
	}

	Message* pb_msg = create_message_by_name(type_name);
	if (pb_msg ==NULL) {
		//free(msg);
		memory_mgr::get_instance().free_memory(msg);
		msg = NULL;
		return false;
	}

	if (false==pb_msg->ParseFromArray(pkg+ BIN_HEAD_LEN, pkg_len- BIN_HEAD_LEN)) {
		//free(msg);
		memory_mgr::get_instance().free_memory(msg);
		msg = NULL;
		delete pb_msg;
		return false;
	}

	msg->body = pb_msg;
	*out_msg = msg;
	return true;
}

bool protoManager::decode_rwa_cmd_msg(unsigned char* pkg, int pkg_len, raw_cmd* raw_out_msg) {
	if (pkg == NULL || pkg_len <= 0 || raw_out_msg==NULL) {
		return false;
	}

	if (pkg_len < BIN_HEAD_LEN) {
		return false;
	}
	//只解析包头
	raw_out_msg->head.stype = pkg[0] + (pkg[1] << 8);
	raw_out_msg->head.ctype = pkg[2] + (pkg[3] << 8);
	raw_out_msg->head.utag = pkg[4] + (pkg[5] << 8) + (pkg[6] << 16) + (pkg[7] << 24);
	raw_out_msg->raw_data = pkg;
	raw_out_msg->raw_len = pkg_len;

	return true;
}

unsigned char* protoManager::encode_cmd_msg(recv_msg* msg, int * out_len) {
	if (msg==NULL) {
		*out_len = 0;
		return NULL;
	}

	unsigned char* package = NULL;
	int total_len = 0;
	if (get_proto_type() == BIN_PROTOCAL) {
		Message* pb_msg = (Message*)msg->body;
		total_len = BIN_HEAD_LEN + pb_msg->ByteSize();
		package = (unsigned char*)malloc(total_len);
		if (package==NULL) {
			*out_len = 0;
			return NULL;
		}
		if (false==pb_msg->SerializePartialToArray(package+ BIN_HEAD_LEN,pb_msg->ByteSize())) {
			*out_len = 0;
			return NULL;
		}

		*out_len = BIN_HEAD_LEN + pb_msg->ByteSize();
	}
	else{
		//json
	}

	//消息头
	package[0] = (msg->head.stype & 0x000000ff);
	package[1] = ((msg->head.stype & 0x0000ff00) >> 8);
	package[2] = (msg->head.ctype & 0x000000ff);
	package[3] = ((msg->head.ctype & 0x0000ff00) >> 8);
	memcpy(package + 4, &msg->head.utag, sizeof(msg->head.utag));
	
	return package;
}

void protoManager::msg_free(recv_msg* msg) {
	if (msg==NULL) {
		return;
	}

	if (get_proto_type() == BIN_PROTOCAL) {
		if (msg->body != NULL) {
			Message* pb_msg = (Message*)msg->body;
			delete pb_msg;
		}
	}
	/*else if (get_proto_type() == JSON_PROTOCAL) {
		free(msg->body);
	}*/

	free(msg);
}