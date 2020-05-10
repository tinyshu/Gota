--��ʼ����־ģ��
Logger_wrapper.init("logger/gateway/", "gateway", true)

--��������
local config = require("conf")
local socket_proto_type = require("socket_proto_type")

--session_wrapper.set_socket_and_proto_type(socket_type.TCP_SOCKET,proto_type.PROTO_BUF)
session_wrapper.set_socket_and_proto_type(socket_proto_type.socket_type.TCP_SOCKET,socket_proto_type.proto_type.PROTO_BUF)
--session_wrapper.set_socket_and_proto_type(socket_type.WEB_SOCKET,proto_type.PROTO_JSON)
--protobufЭ�飬ע��cmd
if session_wrapper.get_proto_type() == socket_proto_type.proto_type.PROTO_BUF then
   local cmd_name_map = require("cmd_name_map")
   if cmd_name_map then
	proto_mgr_wrapper.register_protobuf_cmd(cmd_name_map)
   end
end


--�����������
--tcpģʽ
--netbus_wrapper.tcp_listen(config.gateway_tcp_ip,config.gateway_tcp_port)
--print("start gateway tcp service success ip:".. config.gateway_tcp_ip,"port:"..config.gateway_tcp_port)
--websocketģʽ
netbus_wrapper.websocket_listen(config.gateway_ws_ip,config.gateway_ws_port)
print("start gateway websocket service success ip:".. config.gateway_ws_ip,"port:"..config.gateway_ws_port)


--ע������ת��ģ��
local servers = config.servers
local gate_service = require("gateway_server/gateway_service")
for k,v in pairs(servers) do
	--local ret = service_wrapper.register_service(v.stype, gate_service)
	local ret = service_wrapper.register_raw_service(v.stype, gate_service)
	if ret then
		print("register gw_servce:[" .. v.stype.. "] success!!!")
	else
		print("register gw_servce:[" .. v.stype.. "] failed!!!")
	end
end