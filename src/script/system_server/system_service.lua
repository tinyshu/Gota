local stype = require("service_type")
local Cmd = require("cmd_type")
local utils = require("utils")
local ugame_info_module = require("system_server/user_game_info")
--定义认证服务器协议和函数映射
--在这里注册auth服务处理的协议和对应得人处理函数
local system_service_handles = {}

system_service_handles[Cmd.GetUgameInfoReq] = ugame_info_module.get_ugame_info
-----------------------------------------------

-- {stype, ctype, utag, body}
function on_system_recv_cmd(s, msg)
      print("on_system_recv_cmd")
      utils.print_table(msg)
	--判断cmdid是否有对应的处理函数
	local ctype = msg[2] --协议id
	if system_service_handles[ctype] then
	   system_service_handles[ctype](s,msg)
	end
end

function on_system_session_disconnect(s,ctype) 
   print("on_system_session_disconnect")
end

local system_service = {
      on_session_recv_cmd = on_system_recv_cmd,
      on_session_disconnect = on_system_session_disconnect,
}

return system_service
