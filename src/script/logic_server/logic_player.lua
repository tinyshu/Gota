local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local mysql_game = require("database/mysql_system_ugame")


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
	-- 数据库理面读取玩家的基本信息;
	mysql_game.get_ugame_info(uid, function (err, ugame_info)
		if err then
			ret_handler(res_module.SystemErr) 
			return
		end

		self.ugame_info = ugame_info
		ret_handler(res_module.OK) 
	end)
	-- end

	-- ...其他信息后面再读
	-- end 
end

function player:set_session(s)
	self.session = s
end


return player

