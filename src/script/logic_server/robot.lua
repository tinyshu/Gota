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
