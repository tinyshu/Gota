--初始化日志模块必须放在最前面，避免加载其他模块打log而没有初始化log模块
--true表示写文件同时输出到控制台
Logger_wrapper.init("logger/system/", "system", true)

local socket_proto_type = require("socket_proto_type")
local stype = require("service_type")
local config = require("conf")

session_wrapper.set_socket_and_proto_type(socket_proto_type.socket_type.TCP_SOCKET,socket_proto_type.proto_type.PROTO_BUF)
--session_wrapper.set_socket_and_proto_type(socket_type.TCP_SOCKET,proto_type.PROTO_JSON)
--protobuf协议，注册cmd
if session_wrapper.get_proto_type() == socket_proto_type.proto_type.PROTO_BUF then
   local cmd_name_map = require("cmd_name_map")
   if cmd_name_map then
	proto_mgr_wrapper.register_protobuf_cmd(cmd_name_map)
   end
end


local servers = config.servers
--开启网络服务
local server_ip =  servers[stype.SystemServer].ip
local server_port = servers[stype.SystemServer].port

print("start system service success ip:".. server_ip,"port:"..server_port)
netbus_wrapper.tcp_listen(server_ip,server_port)

local system_service = require("system_server/system_service")
--注册auth服务services模块
print("service_wrapper.register_service call!!")
local ret = service_wrapper.register_service(stype.SystemServer, system_service)
	if ret then
		print("register system_servive:[" .. stype.SystemServer.. "] success!!!")
	else
		print("register system_servive:[" .. stype.SystemServer.. "] failed!!!")
	end