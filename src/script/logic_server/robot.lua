--玩家机器人对象
local player = require("logic_server/logic_player")
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")

--继承player类
local robot = player:new()

function robot:new()
	local instant = {} --类的实例

	setmetatable(instant, {__index = self}) 
	return instant
end

function robot:init(uid, s, ret_handler)
    --调用父类init函数
	player.init(self,uid,s,ret_handler)
	--设置为机器人属性
	self.is_robot = true

end

return robot

