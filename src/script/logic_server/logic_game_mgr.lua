local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")
local zone =   require("logic_server/Zone")

--uid和player的对应关系
local online_player_map = {}
local online_player_num = 0
-- zone_wait_list[Zone.SDYG] = {} --> uid --> p;
--每个地图id作为key, value是另外的一个表match
--match = {uid,player}
local zone_wait_list = {} 

local function send_logic_enter_status(s,uid,sstype,sstatus)	
 
  local ret_msg = {
				   stype=sstype,ctype=cmd_module.LoginLogicRes,utag=uid,
				   body={
							status = sstatus
					    }}
                    
					--utils.print_table(ret_msg)
			        session_wrapper.send_msg(s,ret_msg)
end

--进入游戏逻辑服务器
function login_server_enter(s,msg)
   local uid = msg[3]
   --[[
   	 logic作为对战地图服务器按照这样来划分
	 1.一个logic服务是一个完整的对战服务器，可以支持N个不同的地图和N种不同的玩法
	 2.把不同的地图划分到不同的logic_server上，然后使得每个logic的stype不同，来作为负载的方式
   ]]
   --客户端进入的stype
   local stype = msg[1] 
   print("login_server_enter uid:"..uid.."stype"..stype)
   
   local play = online_player_map[uid]
   if play ~= nil then
      --更新session,这里的session是客户端和gateway的连接对象
      play:set_session(s)
	  send_logic_enter_status(s,uid,stype,res_module.OK)
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
		send_logic_enter_status(s,uid,stype,status)
   end)
end

--网关广播过来的用户断线消息
function on_player_disconnect(s,msg)
    local uid = msg[3]
	print("on_player_disconnect uid:"..uid)
	
	play = online_player_map[uid]
	if play == nil then
	   return
	end

	--判断玩家是否在等待列表
	if play.zid ~= -1 then
	   --移除等待列表
	   zone_wait_list[play.zid][uid] = nil
	   play.zid = -1
	end
	--先不考虑断线重连，直接先重online_player_map删除
	if online_player_map[uid] ~= nil then
	   online_player_map[uid] = nil
	   online_player_num = online_player_num -1
	   if online_player_num < 0 then
	      online_player_num = 0
	   end  
	   print("on_player_disconnect online_player_num:"..online_player_num)
	end

end

--和gateway网关服务断线
--和网关断开后，会有网关侧发起重连请求
function on_gateway_disconnect(s,ctype)
	local k, v
	--这里只删除player对象里存储的session,online_player_map不清除
	--2个原因:
	--1.这里客户端并没有和网关断开，也就是说client和gateway的网关是好的，
	--2.在和gateway断开后，是需要重新去连接网关的，如果连接成功，在把client和gateway
	--连接好的session重新设置到player对象就可以了
    for k, v in pairs(online_player_map) do 
		v:set_session(nil)
	end

end

--gateway连接成功触发这个函数，来通知其他服务service
function on_gateway_connect(s,stype)
	print("on_gateway_connect stype"..stype)
	 for k, v in pairs(online_player_map) do 
		v:set_session(s)
	end

end

function logic_enter_zone(s,msg)
    local stype =  msg[1]
	local uid = msg[3]
	local zid = msg[4].zid
	print("logic_enter_zone stype:"..stype.." uid"..uid)
	if uid <= 0 then
	     local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvaildErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		 return
	end

	--判断进入的地图是否合法
	if zid ~= zone.SGYD and zid ~= zone.ASSY then
		 local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvaildErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		return
	end

	play = online_player_map[uid]
	--找不到或者已经在地图中，无法在进入
	if play == nil or play.zid ~= -1 then
	    local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvalidOptErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		 return
	end

	--第一次添加一个空的列表
	if not zone_wait_list[zid] then
	   zone_wait_list[zid] = {}
	end
	--添加play对象到匹配列表
	play.zid = zid
	zone_wait_list[zid][uid] = play
	local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.ok
					    }}
                    
    session_wrapper.send_msg(s,ret_msg)

end

local game_mgr = {
	login_server_enter = login_server_enter,
	on_player_disconnect = on_player_disconnect,
	on_gateway_disconnect = on_gateway_disconnect,
	on_gateway_connect = on_gateway_connect,
	logic_enter_zone = logic_enter_zone,
}

return game_mgr