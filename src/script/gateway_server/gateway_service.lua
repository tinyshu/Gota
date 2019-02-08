local config = require("conf")
--stype到网关连接的session的session映射
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


function send_to_server(client_session,raw_data)
	local stype,cmd,utag = proto_mgr_wrapper.read_msg_head(raw_data)
	print(stype,cmd,utag)
	local uid = session_wrapper.get_uid(client_session)
	
	local server_session = session_map[stype]
	if server_session == nil then
	print("not found session stype:"..stype)
	   return	
	end
	if uid ==0 then 
		--还未登录
	   utag = session_wrapper.get_utag(client_session)
	   if utag ==0 then
		  --还没有临时的utag
		  utag = g_ukey
		  g_ukey = g_ukey + 1
		  --临时的key和客户端session做一个临时的关系映射
		  client_session_utag[utag] = client_session
		  --设置session的utag值
		  session_wrapper.set_utag(client_session,utag)
	   end
	else
		--已经登录
		utag = uid
		client_session_uid[utag] = client_session
	end
	 
	 --先给数据包写入utag,这样在数据返回发给client_session就有映射关系
	 proto_mgr_wrapper.set_raw_utag(raw_data,utag)
	 --发送数据给stype对应的服务器
	 session_wrapper.send_raw_msg(server_session,raw_data)
end

function send_to_client(server_session,raw_data)
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
	   send_to_server(s,raw_data)
	else
	   --来自服务器
	   send_to_client(s,raw_data)
	end
end

--session断开回调函数
function on_gw_session_disconnect(s) 
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
	end
end

local gw_service = {
	on_session_recv_raw_cmd = on_gw_recv_raw_cmd,
	on_session_disconnect = on_gw_session_disconnect,
}

server_session_init()

return gw_service