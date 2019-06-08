--退出游戏处理模块
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local utils = require("utils")

function login_out_process(s,msg)
	local uid = msg[3]
	print("login_out_process uid:"..uid)

    local ret_msg = {
					    stype=stype_module.AuthSerser,ctype=cmd_module.LoginOutRes,utag=uid,
					    body={
							status = res_module.OK
					}}

    session_wrapper.send_msg(s,ret_msg)
end

local loginout = {
	login_out_process = login_out_process,
}

return loginout