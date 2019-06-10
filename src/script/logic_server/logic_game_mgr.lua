local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")

--uid和player的对应关系
local online_player_map = {}
local online_player_num = 0

local function send_logic_enter_status(s,uid,sstatus)	
 
  local ret_msg = {
				   stype=stype_module.LogicServer,ctype=cmd_module.LoginLogicRes,utag=uid,
				   body={
							status = sstatus
					    }}
                    
					--utils.print_table(ret_msg)
			        session_wrapper.send_msg(s,ret_msg)
end

--进入游戏逻辑服务器
function login_server_enter(s,msg)
   local uid = msg[3]
   print("login_server_enter uid:"..uid)
   
   local play = online_player_map[uid]
   if play ~= nil then
      --更新session
      play:set_session(s)
	  send_logic_enter_status(s,uid,res_module.OK)
	  return
   end

   --没有找到，创建一个player实例
 
   play = player:new()
   if play == nil then
      print("player:new() is error")
	  return
   end

   --重db读取player数据
   
   play:init(uid, s, function(status)
      if status == res_module.OK then
			online_player_map[uid] = play
			online_player_num = online_player_num + 1
			print("online_player_num:"..online_player_num)
		end
		send_logic_enter_status(s,uid,status)
   end)
end

local game_mgr = {
	login_server_enter = login_server_enter,
}

return game_mgr