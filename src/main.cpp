#include <stdio.h>
#include <string>
#include  <stdlib.h>
#include "./utils/logger.h"
#include "../../moduel/net/net_uv.h"
#include "../../moduel/netbus/netbus.h"
#include "../../lua_wrapper/lua_wrapper.h"
#include "./moduel/netbus/session_key_mgr.h"
#include "./moduel/session/tcp_session.h"

int main(int argc, char** argv) {
	//初始化netbus
	init_uv();
	init_session_key_map();
	init_session_manager();
	lua_wrapper::init_lua();
	//lua脚本的搜索路径从./script/开始
	if (argc!=3) {
		std::string search_path = "./script/";
		lua_wrapper::add_search_path(search_path.c_str());
		std::string lua_file = search_path + "auth_server/main.lua";
		int ret = lua_wrapper::exce_lua_file(lua_file.c_str());
		if (ret != 0) {
			exit(0);
		}
	}
	else {
		std::string search_path = argv[1];
		if (*(search_path.end() - 1) != '/') {
			search_path += "/";
		}
		lua_wrapper::add_search_path(search_path.c_str());
		std::string lua_file = search_path + argv[2];
		int ret = lua_wrapper::exce_lua_file(lua_file.c_str());
		if (ret!=0) {
			exit(0);
		}
	}

	run_loop();
	lua_wrapper::exit_lua();
	exit_session_key_map();
}