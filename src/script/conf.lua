--定义整个服务配置
local stype = require("service_type")
                          
--gateway连接的服务在这里添加配置
local remote_server = {}
remote_server[stype.AuthSerser] = {
   stype = stype.AuthSerser,
   ip = "127.0.0.1",
   port = 8000,
   desic = "AuthSerser",
}

--[[
remote_server[stype.SystemServer] = {
   stype= stype.SystemServer,
   ip= "127.0.0.1",
   port= 8001,
   desic = "SystemServer",
}

]]

local conf = {
    --tcp监听配置
	gateway_tcp_ip = "127.0.0.1",
	gateway_tcp_port = 6080,
	--websocket监听配置
	gateway_ws_ip = "127.0.0.1",
	gateway_ws_port = 6081,
	--定义远程gateway需要连接的配置信息
	servers = remote_server,
	
	--认证服务器mysql
	auth_mysql= {
	  host = "123.206.46.126",      port = 3306,      db_name = "auth_center",      uname = "root",      upwd = "123321",
	},
}

return conf