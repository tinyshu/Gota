local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local mysql_game = require("database/mysql_system_ugame")
local redis_center = require("database/redis_auth_center")
local room_status = require("logic_server/room_status")

--定义一个对象player类
local player = {}

function player:new(instant) 
	if not instant then 
		instant = {} --类的实例
	end

	setmetatable(instant, {__index = self}) 
	return instant
end


function player:init(uid, s, ret_handler)
	self.session = s
	self.uid = uid
	--zid玩家进入的地图，初始化为-1,表示为不在任何地图，
	self.zid = -1
	--房间唯一标识符
	self.roomid = -1
	--匹配的作为id
	self.seatid = -1
	--玩家属于的队伍 0左边或1右边
	self.side= -1
	--玩家选用的英雄 暂时取值[1-5]
	self.heroid = -1
	--用户在房间里的状态
	self.status = room_status.InView
	--是否为机器人玩家
	self.is_robot = false
	self.client_udp_ip = nil
	self.client_udp_port = 0
	--当前同步到哪一帧
	self.cur_sync_frame_id = 0   --当前同步到那一帧
	-- 数据库理面读取玩家的基本信息;
	mysql_game.get_ugame_info(uid, function (err, ugame_info)
		if err then
		    if ret_handler then
			   ret_handler(res_module.SystemErr) 
			end
			return
		end

		self.ugame_info = ugame_info
		--ret_handler(res_module.OK) 

	    -- ...其他信息后面再读
	    --redis读取用户信息 主要获取unkci,uface,usex
	    redis_center.get_userinfo_to_redis(self.uid,function(err,user_uinfo)
	    if err ~= nil then
		   print("get_userinfo_to_redis err:"..err)
		   if ret_handler then
		     ret_handler(res_module.SystemErr)
		   end	  
		   return
		end

		--print("play read redis unick:"..user_uinfo.unick.." uface:"..user_uinfo.uface)
		self.uinfo = user_uinfo
		if ret_handler then
		   ret_handler(res_module.OK)
		end 
    end)
 end)
	-- end
end

function player:set_udp_addr(ip,port)
	if ip == nil or port<=0 then
	   print("set_udp_addr invalid parament")
	   return
	end

	self.client_udp_ip = ip
	self.client_udp_port = port
	--print("------------player:set_udp_addr ip:"..self.client_udp_ip.." port:"..self.client_udp_port)
end

function player:set_session(s)
	self.session = s
end

function player:send_cmd(sstype, cctype, cbody)
	if not self.session or self.is_robot==true then 
		return
	end

	local ret_msg = {stype = sstype,ctype = cctype,utag = self.uid, body=cbody}
	--utils.print_table(ret_msg)
    session_wrapper.send_msg(self.session,ret_msg)
end

function player:udp_send_cmd(sstype, cctype, cbody)
	if not self.session or self.is_robot==true then 
		return
	end

	if self.client_udp_ip == nil or self.client_udp_port == 0 then
	    --print("client_udp_ip nil or client_udp_port==0")
	    return
	end
	local ret_msg = {stype = sstype,ctype = cctype,utag = self.uid, body=cbody}
	--utils.print_table(ret_msg)
	session_wrapper.udp_send_msg(self.client_udp_ip,self.client_udp_port,ret_msg)
end

--这里返回给房间里用户信息，需要什么在这里添加
function player:get_user_arrived()
	local body = {
	    --在房间里展示的信息
		unick = self.uinfo.unick,
		uface = self.uinfo.uface,
		usex =  self.uinfo.usex,
		seatid = self.seatid,
		side = self.side,
	}

	return body
end

return player

