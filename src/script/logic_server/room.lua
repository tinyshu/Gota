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
	self.frameid = 0          --当前帧id
	self.inview_players = {}  -- 旁观玩家的列表
	self.lhs_players = {}     -- 左右两边的玩家
	self.rhs_players = {}     -- 左右两边的玩家

	self.all_frame_list = {}  --比赛开始以来所有的帧操作 
	self.next_frame_list = {}  --当前frameid对应的帧操作(这里是需要接受的下一帧)

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
		   --print("random heroid:"..p.heroid.." uid:"..p.uid)
		   break
		end 
	end
	p.roomid = self.room_id
	p.cur_sync_frame_id = 1
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
		--更新玩家状态
	    self:update_play_status(status.Ready)

		--全部都read状态后，发送开始游戏通知给房间内用户
		self:game_start()
	end
	--print("room:enter_room end")
	return true
end

function room:update_play_status(status)
	local k,v
	for k,v in pairs(self.inview_players) do 
		if v ~= nil then
		   v.status = status
	    end
	end

end

--全部都read状态后，发送开始游戏通知给房间内用户
--这里缺少给用户选hero的步骤,直接给用户随机选择[1-5]一个英雄角色 
function room:game_start()
	--local heroes = {}
	local players_match_info = {}
	for i = 1, PLAYER_NUM_3v3 * 2 do
	    if self.inview_players[i] ~= nil then
		   local p = self.inview_players[i]
		   local info = {
			heroid = p.heroid,
			seatid = p.seatid,
			side = p.side,
		}

		table.insert(players_match_info, info)
		--table.insert(heroes, self.inview_players[i].heroid)
		end
	end
	--房间内全部玩家匹配信息
	local msgbody = {
	      players_match_info = players_match_info,
	}
	
	self:broadcast_cmd_inview_players(stype_module.LogicServer, cmd_module.GameStartNotify, msgbody, nil)
	--通知游戏开始后，状态设置为Start
	self.room_state = status.Playing
	self:update_play_status(status.Playing)

	--从第一帧开始
	self.frameid = 1
    self.all_frame_list = {}  --比赛开始以来所有的帧操作 
	self.next_frame_list = {frameid=self.frameid,opts = {}}  --当前frameid对应的帧操作(待接受的下一帧)
	--客户端在收到GameStartNotify消息后会开始loading加载资源
	--等待3s后，开始帧同步，20fps,每次间隔就是50ms(启动一个间隔50ms定时执行的定时器)
	--注意这里create_timer需要碘绑定一个匿名函数，在匿名函数里面在调用类成员函数 
	self.frame_time = timer_wrapper.create_timer(function()
           self:do_logic_frame_sync()
		   end,-1,3000,50)

end

function room:push_next_frame(next_frame_opts)
    local seatid = next_frame_opts.seatid
	--print(seatid, next_frame_opts.frameid, #next_frame_opts.opts)

	local p = self.inview_players[seatid]
	if not p then 
		return
	end

	--g更新play当前同步帧
	if p.cur_sync_frame_id <  next_frame_opts.frameid - 1 then
	   p.cur_sync_frame_id = next_frame_opts.frameid - 1
	end

	--判断当前传入的帧是否是和服务器代收的frameid匹配
    if  next_frame_opts.frameid ~= self.frameid then
	    print("next_frame_opts.frameid ~= self.frameid")
		return
	end

	for i = 1, #next_frame_opts.opts do 
		table.insert(self.next_frame_list.opts, next_frame_opts.opts[i])
	end

end

--发送为同步的帧
function room:send_unsync_frame(p)
     if p == nil then
	    return 
	 end
	
	 if #self.all_frame_list == 0 then
	    return
	 end
	 --给玩家同步帧(只同步未同步的帧集合)
	 local unsync_opt_frame = {}  --存储当前未同步的帧 
     local i
	 --从play的当前帧到room的最后帧同步给玩家
	 for i = (p.cur_sync_frame_id + 1),#self.all_frame_list do
	      table.insert(unsync_opt_frame, self.all_frame_list[i])
	 end
	 --更新用户帧frameid??
	 --
	 --p.cur_sync_frame_id = self.frameid
	 if #unsync_opt_frame > 0 then
	     local body = {frameid = self.frameid,unsync_frames = unsync_opt_frame}
         p:udp_send_cmd(stype_module.LogicServer, cmd_module.LogicFrame,body)      
	 end
    
end

function room:do_logic_frame_sync()
    --这一帧的数据，存储到all_frame_list
	table.insert(self.all_frame_list,self.next_frame_list)

	--便遍历全部玩家，这这一帧的数据发送出去
	local k,p
	for k,p in pairs(self.inview_players) do 
		if p then
		  self:send_unsync_frame(p)
		end
	end

	--服务器当前帧，也是需要接受的下一个frameid
	self.frameid = self.frameid + 1
	--下一帧数据覆盖掉上一帧
	self.next_frame_list = {frameid = self.frameid,opts = {}}
	--print("do_logic_frame_sync self.frameid: "..self.frameid)
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

