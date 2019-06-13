local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")
local zone =   require("logic_server/Zone")
local room_moduel = require("logic_server/room")
local room_status = require("logic_server/room_status")

local mysql_game = require("database/mysql_system_ugame")
local redis_game = require("database/redis_game")
--中心服务存储接口
--中心服务存储接口
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local robot_player = require("logic_server/robot")
--uid和player的对应关系
local online_player_map = {}
local online_player_num = 0

--匹配列表zone_wait_list
-- zone_wait_list[Zone.SGYD] = {} --> uid --> p;
--每个地图id作为key, value是另外的一个表match
--match = {uid,player}
local zone_wait_list = {} 

--匹配成功比赛房间列表,也是按照zid来划分
--room_list[zid]获取全部zid地图的房间列表list  表格式: {zid,{roomid,room}}
local room_list = {}
room_list[zone.SGYD] = {}
room_list[zone.ASSY] = {}

--机器人列表,也是按照地图涵划分 表格式{zid,{uid,borot}}
local zone_robot_list = {}
zone_robot_list[zone.SGYD] = {}
zone_robot_list[zone.ASSY] = {}

local function do_new_robot_play(robotlist)
  
	if #robotlist <=0 then
	   return
	end

	--根据地图来划分robot
	local half_len = #robotlist
	local i = 1
	half_len = math.floor(half_len * 0.5)
	print("half_len:"..half_len)
	--创建robot对象
    --for i = 1, half_len do 
	--先不划分，把全部robot放到一个地图
	for i = 1, #robotlist do 
	    local v = robotlist[i] 
		local robot = robot_player:new()
		robot:init(v.uid,nil,nil)
		robot.zid = zone.SGYD
		zone_robot_list[zone.SGYD][v.uid] = robot
		
	end
	--[[
	   for i = half_len + 1, #robotlist do 
	    local v = robotlist[i] 
		local robot = robot_player:new()
		robot:init(v.uid,nil,nil)
		robot.zid = zone.ASSY
		zone_robot_list[zone.ASSY][v.uid] = robot
		print("zone_robot_list[zone.ASSY][v.uid] = robot")
	end
	]]
	

end

function do_load_robot_uinfo(now_index, robots)
    mysql_center.get_userinfo_by_uid(robots[now_index].uid, function (err, uinfo)
    if err or not uinfo then
	        print("get_userinfo_by_uid error"..err)
			return 
	end

	redis_center.set_userinfo_to_redis(robots[now_index].uid, uinfo)
	--print("uid " .. robots[now_index].uid .. " load to center reids!!!")
	now_index = now_index + 1
	if now_index > #robots then
	   --到这里说明全部robot数据加载完成
	   --创建robot对象，并存储到zone_robot_list列表
	   do_new_robot_play(robots)
	   return 
	end

	do_load_robot_uinfo(now_index, robots)
	end)
end

local function search_idle_robot(zid)
	local robots = zone_robot_list[zid]
	local k, v 
	for k, v in pairs(robots) do 
		if v.roomid == -1 then
			return v 
		end
	end

	return nil
end

--由于定时器触发，来把robot放入到比赛中
function do_push_to_match()
   
	--遍历全部在inview状态下的房间,然后在找到一个robot对象，添加进房间
    local zid, zid_room_list
	local k, room  
	for zid, zid_room_list in pairs(room_list) do 
	    
		for k, room in pairs(zid_room_list) do
			if room.room_state == room_status.InView then  -- 找到了一个空闲的room
				--找到一个可以加入的robot
				--print("room state is InView zid"..zid)
				local robot = search_idle_robot(zid)
				if robot then
					print("[".. robot.uid .."]" .. " enter match!")
					robot.zid = zid
					room:enter_room(robot) 
				end
			end 
		end
	end

end

--启动一个1s执行一次的定时任务 ，如果不需要robot模式
timer_wrapper.create_timer(do_push_to_match,-1,2000,1000)

function do_load_robot_ugame_info()
 
	mysql_game.get_robots_ugame_info(function(err, ret)
	    if err then
		    print("get_robots_ugame_info err"..err)
			return 
		end

		--没有数据
		if not ret or #ret <= 0 then 
			return
		end

		--把数据添加到redis
		local k, v
		for k, v in pairs(ret) do
			redis_game.set_ugame_info_inredis(v.uid, v)
		end

		--把用户中心信息设置到game数据
		do_load_robot_uinfo(1, ret)
	end)
end

function load_robots()
    --print("load_robots")
	if not mysql_game.is_connectd() or 
	   not mysql_center.is_connectd() or 
	   not redis_center.is_connectd() or not redis_game.is_connectd() then 
	       --print("create_timer_once")
		   --启动一个一次性定时任务
	       timer_wrapper.create_timer_once(load_robots,1000,1000)
	       return
	end

	--读取robot数据
	do_load_robot_ugame_info()
end

load_robots()


--找到一个状态是InView的房间
local function search_inview_match_mgr(zid)
    if zid < 0 then
	   return nil
	end
	
	--获取zid地图全部list
	zid_room_list = room_list[zid]
	if zid_room_list == nil then
	    print("search_inview_match_mgr zid_room_list is nil")
	    return nil
	end
    
	local k,v
	for k,v in pairs(zid_room_list) do
	   if v.room_state == room_status.InView then
	      return v
	   end

	   --判断当前人数是否已满
	end
	--找不到,创建一个新room对象返回
	local croom = room_moduel:new()
	croom:init(zid)
	table.insert(zid_room_list,croom)
	return croom
end

--匹配定时器，周期判断等待列表zone_wait_list匹配逻辑
--一个定时器调用函数处理一个地图的匹配
function do_match_sgyd_map()
 
	--等待zone.SGYD地图的全部玩家列表
	if #zone_wait_list == 0  then
	   return
	end

	local zid, wait_list
	--遍历所有的等待列表
	for zid, wait_list in pairs(zone_wait_list) do 
	    local k,v
	    for k,v in pairs(wait_list) do
	        --k:uid v:player
	        local room =  search_inview_match_mgr(zid)
	        if room then
	           if not room:enter_room(v) then
			      --出错
				  print("room:enter_room error! v.status:"..v.status)
			   else
			      --加入成功，从等待列表移除
				  print("add success wait_list remove! uid:"..k)
				  zone_wait_list[zid][k] = nil
			   end
		    else
			   print("room is nil")
	        end
	    end
	end

end


--1s一次匹配,注意这一行需要放到函数定义下面
init_count = 0
if init_count==0 then
   timer_wrapper.create_timer(do_match_sgyd_map,-1,1000,1000)
   init_count = init_count + 1
end


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
	        if online_player_map[uid] == nil then
			   online_player_map[uid] = play
			   online_player_num = online_player_num + 1
			   print("...online_player_num:"..online_player_num)
			end
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
	   print("on_player_disconnect online_player_map remove uid:"..uid)
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
	--print("on_gateway_connect stype"..stype)
	 for k, v in pairs(online_player_map) do 
		v:set_session(s)
	end

end

function logic_enter_zone(s,msg)
    local stype =  msg[1]
	local uid = msg[3]
	local zid = msg[4].zid
	print("logic_enter_zone stype:"..stype.." uid:"..uid.." zid:"..zid)
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
	    print("play.zid:"..play.zid)
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
	print("zone_wait_list[zid][uid] zid:"..zid.." uid:"..uid)
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