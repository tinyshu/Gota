local config = require("conf")
local stype_module = require("service_type")

local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")

--47min uid管理写完，还未测试
--stype到网关连接的session的session映射,存储的连接的服务器的session
local session_map = {}
--标记正在连接还未成功的session
local session_connecting = {}

local g_ukey = 1;
--临时utag映射session，这个key有lua生成
local client_session_utag = {}
--uid映射session
local client_session_uid = {}

function connect_to_server(stype,server_ip,server_port)
	netbus_wrapper.tcp_connect(server_ip,server_port,function(err,session)
		--连接失败
		if err ~= nil then
	       session_connecting[stype] = false
		   print("connect error server["..config.servers[stype].desic.."]".."error:"..err)
		   return
	    end
		--连接成功
		session_map[stype] = session
		print("connect sucess server["..config.servers[stype].desic.."]")
end
)
end

function check_session_connect()
	--遍历网关需要连接的服务器
	for k,v in pairs(config.servers) do
		if session_map[v.stype] == nil and session_connecting[v.stype] == false then
			session_connecting[v.stype] = true
			print("connecting server["..v.desic.."]"..v.ip..":"..v.port)
			connect_to_server(v.stype,v.ip,v.port)
		end
	end
end

function server_session_init()
	for k,v in pairs(config.servers) do
		session_map[v.stype] = nil
		session_connecting[v.stype] = false
	end
	--启动连接定时器
	timer_wrapper.create_timer(check_session_connect,-1,1000,1000)
end

--local socket_type = WEB_SOCKET
--local proto_type =  PROTO_JSON
function is_login_request(cmd_type)
	if cmd_type == cmd_module.GuestLoginReq then 
		return true
	end

	return false
end

--客户端发送过来，由网关服务器转发给服务器
function send_to_server(client_session,raw_data)
	local stype,cmd,utag = proto_mgr_wrapper.read_msg_head(raw_data)
	print(stype,cmd,utag)
	
	--获取服务器连接的session
	local server_session = session_map[stype]
	if server_session == nil then
	print("not found session stype:"..stype)
	   return	
	end
	
	--判断是否为登录协议请求
	if is_login_request(cmd) then
	  utag = session_wrapper.get_utag(client_session)
	   --还没有utag，生成一个tag,在登录前使用
	   if utag ==0 then
		  --还没有临时的utag
		  utag = g_ukey
		  g_ukey = g_ukey + 1
		 
		  --设置session的utag值
		  session_wrapper.set_utag(client_session,utag)
	   end
		   --临时的key和客户端session做一个临时的关系映射
	       client_session_utag[utag] = client_session
		else
		 --这里处理不是登录请求
		 local uid = session_wrapper.get_uid(client_session)
		 utag = uid
	end
	 
	 --先给数据包写入utag,这样在数据返回发给client_session就有映射关系
	 proto_mgr_wrapper.set_raw_utag(raw_data,utag)
	 --发送数据给stype对应的服务器
	 session_wrapper.send_raw_msg(server_session,raw_data)

end

--判断是否为登录返回协议
function is_login_ctype(ctype)
	if ctype == cmd_module.GuestLoginRes then
		return true
	end
	return false
end

--服务器发过来的信息，转给对应的客户端
function send_to_client(server_session,raw_data)
	local cmdtype, ctype, utag = proto_mgr_wrapper.read_msg_head(raw_data)
	print(utag)
	local client_session = nil
	
	--判断是否为登录返回协议
	print("send_to_client ctype,"..ctype)
	if is_login_ctype(ctype) == true then
	    print("is_login_ctype")
		--登录协议返回，在这里读取认证服务器返回的uid
		local t_body = proto_mgr_wrapper.read_msg_body(raw_data)
		if t_body == nil then
		   print("t_body is nil")
		   return
		end
		--if true == true then
		--    return
		--end

		client_session = client_session_utag[utag]
		if client_session==nil then
			print("client_session is nil")
			return
		end
		if t_body.status ~= res_module.OK then
		   
		   proto_mgr_wrapper.set_raw_utag(raw_data,0)
		   if client_session ~= nil then
			  session_wrapper.send_raw_msg(client_session,raw_data)
			end
		end
		--local ret_msg = {stype=stype.AuthSerser,ctype=2,utag=msg[3],body={status=200}}
		local t_userinfo = t_body.userinfo
		local login_uid = t_userinfo.uid
		print("login_uid"..login_uid)
		if login_uid~= 0 then 
		   --判断是否有session是否已经登录
		   if client_session_uid[login_uid] ~= nil and client_session_uid[login_uid] ~= client_session then
		       print("Relogin uid is"..login_uid)
		       --返回一个重复登录消息,新加一个重复登录协议21
			  local ret_msg = {stype=stype_module.AuthSerser,ctype=cmd_module.ReLoginRes,utag=0,body=nil}
			  session_wrapper.send_msg(client_session,ret_msg)
			   --删除已经存在的session
			   session_wrapper.close_session(client_session)
			   client_session_uid[login_uid] = nil
		   end 
		end

		--先把登录前的utag表里的utag对应的值设置为空
		client_session_utag[utag] = nil
		client_session_uid[login_uid] = client_session
		--登录成功后，在网关设置uid到底层session对象
		print("client_session_uid uid="..login_uid)
		session_wrapper.set_uid(client_session,login_uid)
		--这里设置为0,主要是为了不暴露uid给客户端
		t_body.userinfo.uid = 0
		--返回登录请求的userinfo信息给前端
		print("t_body is")
		utils.print_table(t_body)
		local ret_msg = {stype=stype_module.AuthSerser,ctype=cmdtype,utag=0,body=t_body}
		session_wrapper.send_msg(client_session,ret_msg)
		return
	end

	--根据登录前，还是登陆后获取client_session
	if client_session_utag[utag] ~= nil then
		print("client_session_utag")
		client_session = client_session_utag[utag]
	elseif client_session_uid[utag] ~= nil then
	    print("client_session_uid")
		client_session = client_session_uid[utag]
	else 
	    print("not fonud client session")
	end

	--把协议里的utag重置为0 
	proto_mgr_wrapper.set_raw_utag(raw_data,0)
	if client_session ~= nil then
	   --转发数据给客户端
	   --print("send_raw_msg client session")
	   session_wrapper.send_raw_msg(client_session,raw_data)
	else
		print("send_to_client: not found clcient session")
	end
end

-- raw_data由C++测那个推送的完整数据包
--网关可能收到2中session类型数据
--来自客户端，需要根据stype转发给对应的session
--来自服务器，根据utag或者uid转发给客户端对应的session
function on_gw_recv_raw_cmd(s, raw_data)
	print("on_gw_recv_raw_cmd")
	is_client_session = session_wrapper.is_client_session(s)
	if is_client_session==0 then 
	   --来自客户端数据
	   --print("send_to_server")
	   send_to_server(s,raw_data)
	else
	   --来自服务器
	  --print("send_to_client")
	   send_to_client(s,raw_data)
	end
end

--session断开回调函数
function on_gw_session_disconnect(s,service_stype) 
	print("on_gw_session_disconnect!!")
    --这个是网关连接其他服务器的session
	if session_wrapper.is_client_session(s)==1 then
	--网关连接的session断开回调函数
		--print(s)
		for k,v in pairs(session_map) do
			--print(v)
			if v == s then
				--print("disconnect server["..config.servers[k].desic.."]")
				session_map[k] = nil
				session_connecting[k] = false
				return
			end
		end
		return
	end
	--print("on_gw_session_disconnect BBB!!")
	--客户端连接到网关的session,这里是登录前
	local utag = session_wrapper.get_utag(s)
	--print("on_gw_session_disconnect BBB!!"..utag)
	if client_session_utag[utag] ~= nil  and client_session_utag[utag] == s then
	   print("client_session_utag[utag] remove!!")
	   client_session_utag[utag] = nil --这句话能保证utag对应的数据被删除，不在使用remove
	   session_wrapper.set_utag(s,0)
	end

	--客户端连接到网关，已经是登录后
	local uid = session_wrapper.get_uid(s)
	if client_session_uid[uid] ~= nil and client_session_uid[uid] == s then
	   print("client_session_uid[uid] remove!!")
	   client_session_uid[uid] = nil
	end

	--客户端断线，网关负责通知给其他服务器
	local server_session = session_map[service_stype]
	if server_session == nil then
		return
	end
	
	if uid~=0 then
	  --广播用户断线的消息给，所有连接的网关
	  local ret_msg = {stype=service_stype,ctype=cmd_module.UserLostConn,utag=uid,body=nil}
	  session_wrapper.send_msg(server_session,ret_msg)
	end
end

local gw_service = {
	on_session_recv_raw_cmd = on_gw_recv_raw_cmd,
	on_session_disconnect = on_gw_session_disconnect,
}

server_session_init()

return gw_service