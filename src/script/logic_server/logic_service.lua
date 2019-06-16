local stype = require("service_type")
local Cmd = require("cmd_type")
local logic_game_mgr = require("logic_server/logic_game_mgr")
local utils = require("utils")
--定义认证服务器协议和函数映射
--在这里注册logic服务处理的协议和对应得人处理函数

local logic_service_handles = {}
logic_service_handles[Cmd.LoginLogicReq] = logic_game_mgr.login_server_enter
logic_service_handles[Cmd.UserLostConn] =  logic_game_mgr.on_player_disconnect
logic_service_handles[Cmd.EnterZoneReq] =  logic_game_mgr.logic_enter_zone
logic_service_handles[Cmd.ExitRoomReq] =   logic_game_mgr.ExitRoomReq
logic_service_handles[Cmd.NextFrameOpts] = logic_game_mgr.on_next_fream_recv
-----------------------------------------------
-- {stype, ctype, utag, body}
function on_logic_recv_cmd(s, msg)
	--解析数据做响应的逻辑
	  --print("on_logic_recv_cmd ctype:"..msg[2])
      --utils.print_table(msg)
	--判断cmdid是否有对应的处理函数
	local ctype = msg[2] --协议id
	if logic_service_handles[ctype] then
	   logic_service_handles[ctype](s,msg)
	else
	    print("not found function ctype:"..ctype )
	end
end

function on_session_connect(s,stype)
	--print("gateway connect to Logic !!!")
	logic_game_mgr.on_gateway_connect(s,stype)
end

--当和网关连接断开，函数被调用
function on_logic_session_disconnect(s,ctype) 
    print("on_gateway_session_disconnect!! ctype:"..ctype)
	--当网关断开
	logic_game_mgr.on_gateway_disconnect(s,ctype)
end

local logic_service = {
      on_session_recv_cmd = on_logic_recv_cmd,
      on_session_disconnect = on_logic_session_disconnect,
	  	--gateway重连其他服务器成功,调用
	  on_session_connect = on_session_connect,
}

return logic_service
