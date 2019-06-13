--房间模块
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")
local zone =   require("logic_server/Zone")
local status = require("logic_server/room_status")
--当前放假全局id
local g_room_id = 1
--定义一边的参与人数,一局的人数就是*2
local PLAYER_NUM_3v3 = 2
local PLAYER_NUM_5v5 = 5

--定义一个room对象类
local room = {}

function room:new(instant)
	if not instant then
	   instant = {}
	end

	--合并元表
	setmetatable(instant,{__index=self})
	return instant
end

function room:init(zid)

    self.zid = zid
	self.room_id = g_room_id
	roomid = g_room_id + 1
	self.room_state = status.InView
	 
	self.inview_players = {}  -- 旁观玩家的列表
	self.lhs_players = {}     -- 左右两边的玩家
	self.rhs_players = {}     -- 左右两边的玩家
end

function room:exit_room(p)
	
	if p == nil then
	   print("exit p is null")
	   return false
	end
    
	--广播其他用户我退出房间了
	local msgbody = {
	    seatid = p.seatid,
	}
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.UserExitRoomNotify, msgbody, p)

	--先从自己等待匹配列表移除
	if self.inview_players[p.seatid] ~= nil then
	   self.inview_players[p.seatid] = nil   
	end
	
	--重置状态后 通知玩家，退出成功
	p.zid = -1;
	p.matchid = -1
	p.seatid = -1
	p.side = -1
	local body = {status = res_module.OK}
	p:send_cmd(stype_module.LogicServer, cmd_module.ExitRoomRes, body)
	
end

--玩家进入放假处理函数
function room:enter_room(p)

    if p == nil then
	   print("enter p is null")
	   return false
	end 
	
	--判断房间和玩家状态是否合法
	--print("room_status:"..self.room_state.."pstatus"..p.status)
	if self.room_state ~= status.InView or p.status ~= status.InView then 
	    print("room:enter_room return false")
		return false
	end

	--table.insert(self.inview_players,p)
	--查找一个空闲的seatid
	local i
	for i = 1,PLAYER_NUM_3v3*2 do
	    if self.inview_players[i] == nil then
		   self.inview_players[i] = p
		   p.seatid = i
		   --设置队伍信息
		   if i < PLAYER_NUM_3v3  then
		      p.side = 0
		   else
		      p.side = 1
		   end

		   --给用户随机一个heroid
		   p.heroid = math. floor(math.random()*5 + 1)
		   print("random heroid:"..p.heroid.." uid:"..p.uid)
		   break
		end 
	end
	p.roomid = self.room_id
	--通知房间里等待的玩家，我进来了
	--[[local ret_msg = {
	                  --这里传stype_module.LogicServer还有点问题,如果是多区域划分，这里就不能直接传入
				      stype=stype_module.LogicServer,ctype=cmd_module.EnterPlayNotify,utag=uid,
				      body={
							
	}}]]
                  
    local body = { 
		zid = self.zid,
		roomid = self.room_id,
		seatid = p.seatid,
		side = p.side,
	}  
	--这个消息是通知给自己，加入房间等待列表返回的消息
	 p:send_cmd(stype_module.LogicServer,cmd_module.EnterPlayNotify,body)	 
	
	--通知房间内等待的其他用户，你也进入等待状态
	msgbody = p:get_user_arrived()
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.EnterArriveNotify, msgbody, p)
	
	--已经在房间里玩家的数据，通知给刚进入房间的玩家
	local k,v
	--for i = 1, #self.inview_players do
	  for k,v in pairs(self.inview_players) do 
		--有数据并且不是刚刚进入的玩家，就发起通知
		if v ~= nil and v ~= p then 
			local body = v:get_user_arrived()
			p:send_cmd(stype_module.LogicServer, cmd_module.EnterArriveNotify, body)
		end
	end

	-- 判断我们当前是否集结玩家结束了
	if #self.inview_players >= PLAYER_NUM_3v3 * 2 then 
	    print("room is read!!!")
	    self.room_state = status.Ready
	   	local k,v
		for k,v in pairs(self.inview_players) do 
		    if v ~= nil then
			   v.state = status.Ready
			end
		end

		--全部都read状态后，发送开始游戏通知给房间内用户
		self:game_start()
	end
	--print("room:enter_room end")
	return true
end

--全部都read状态后，发送开始游戏通知给房间内用户
--这里缺少给用户选hero的步骤,直接给用户随机选择[1-5]一个英雄角色 
function room:game_start()
	local heroes = {}
	for i = 1, PLAYER_NUM_3v3 * 2 do
	    if self.inview_players[i] ~= nil then
		   table.insert(heroes, self.inview_players[i].heroid)
		end
	end

	local msgbody = {
		heroes = heroes,
	}
	
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.GameStartNotify, msgbody, nil)
end

function room:broadcast_cmd_inview_players(cstype, cctype, cbody, not_to_player)
	local k,v 
	--for i = 1, #self.inview_players do 
	for k,v in pairs(self.inview_players) do 
		if v ~= not_to_player then 
			v:send_cmd(cstype, cctype, cbody)
		end
	end
end


return room

