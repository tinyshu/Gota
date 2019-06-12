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
	--zid玩家进入的地图，初始化为-1
	self.zid = -1
	--房间唯一标识符
	self.roomid = -1
	--用户在房间里的状态
	self.status = room_status.InView
	-- 数据库理面读取玩家的基本信息;
	mysql_game.get_ugame_info(uid, function (err, ugame_info)
		if err then
			ret_handler(res_module.SystemErr) 
			return
		end

		self.ugame_info = ugame_info
		--ret_handler(res_module.OK) 

	    -- ...其他信息后面再读
	    --redis读取用户信息 主要获取unkci,uface,usex
	    redis_center.get_userinfo_to_redis(self.uid,function(err,user_uinfo)
	    if err ~= nil then
		   print("get_userinfo_to_redis err:"..err)
		   ret_handler(res_module.SystemErr) 
		   return
		end

		print("play read redis unick:"..user_uinfo.unick.." uface:"..user_uinfo.uface)
		self.uinfo = user_uinfo
		ret_handler(res_module.OK) 
    end)
 end)
	-- end



end

function player:set_session(s)
	self.session = s
end

function player:send_cmd(sstype, cctype, cbody)
	if not self.session then 
		return
	end

	local ret_msg = {stype = sstype,ctype = cctype,utag = self.uid, body=cbody}
	utils.print_table(ret_msg)
    session_wrapper.send_msg(self.session,ret_msg)
end


function player:get_user_arrived()
	local body = {
	    --在房间里展示的信息
		unick = self.uinfo.unick,
		uface = self.uinfo.uface,
		usex =  self.uinfo.usex,
	}

	return body
end

return player

