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
local PLAYER_NUM_3v3 = 3
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

	print("room:init(zid)")
end

function room:enter_room(p)

	print("room:enter_room start")
    if p == nil then
	   print("enter p is null")
	   return false
	end 
	--判断房间和玩家状态是否合法
	print("room_status:"..self.room_state.."pstatus"..p.status)
	if self.room_state ~= status.InView or p.status ~= status.InView then 
	    print("room:enter_room return false")
		return false
	end

	table.insert(self.inview_players,p)
	p.roomid = self.room_id
	--通知房间里等待的玩家，我进来了
	--[[local ret_msg = {
	                  --这里传stype_module.LogicServer还有点问题,如果是多区域划分，这里就不能直接传入
				      stype=stype_module.LogicServer,ctype=cmd_module.EnterPlayNotify,utag=uid,
				      body={
							
	}}]]
                  
    local body = { 
		zid = self.zid,
		roomid = self.room_id
	}  
	--这个消息是通知给自己，加入房间等待列表返回的消息
	 p:send_cmd(stype_module.LogicServer,cmd_module.EnterPlayNotify,body)	 
	
	--通知房间内等待的其他用户，你也进入等待状态
	msgbody = p:get_user_arrived()
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.EnterArriveNotify, msgbody, p)
	
	--已经在房间里玩家的数据，通知给刚进入房间的玩家
	for i = 1, #self.inview_players do 
		if self.inview_players[i] ~= p then 
			local body = self.inview_players[i]:get_user_arrived()
			p:send_cmd(stype_module.LogicServer, cmd_module.EnterArriveNotify, body)
		end
	end

	-- 判断我们当前是否集结玩家结束了
	if #self.inview_players >= PLAYER_NUM_3v3 * 2 then 
	   self.room_state = status.Ready
	   	for i = 1, #self.inview_players do 
			self.inview_players[i].state = status.Ready
		end
	end
	print("room:enter_room end")
	return true
end

function room:broadcast_cmd_inview_players(cstype, cctype, cbody, not_to_player)
	local i 
	for i = 1, #self.inview_players do 
		if self.inview_players[i] ~= not_to_player then 
			self.inview_players[i]:send_cmd(cstype, cctype, cbody)
		end
	end
end


return room

