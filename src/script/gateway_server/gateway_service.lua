local config = require("conf")
--stype到网关连接的session的session映射
local session_map = {}
--标记正在连接还未成功的session
local session_connecting = {}

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

-- raw_data由C++测那个推送的完整数据包
function on_gw_recv_raw_cmd(s, raw_data)
end

--session断开回调函数
function on_gw_session_disconnect(s) 
	if session_wrapper.is_client_session(s)==1 then
	--网关连接的session断开回调函数
		print("disconnect server A")
		print(s)
		for k,v in pairs(session_map) do
			print(v)
			if v == s then
			print("disconnect server B")
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