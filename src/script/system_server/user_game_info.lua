--系统服务模块
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_game = require("database/mysql_system_ugame")
local utils = require("utils")

--获取游戏数据
function get_ugame_info(s, msg)
   local uid = msg[3];
   print("get_ugame_info call"..uid)
   
   mysql_game.get_ugame_info(uid, function (err, uinfo) 
       if err then -- 告诉客户端系统错误信息;
			 local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		end

		--没有查询到，说明是首次进入，系统自动初始化一条数据
		if uinfo == nil then 
			mysql_game.insert_ugame_info(uid, function(err, ret)
				if err then -- 告诉客户端系统错误信息;
					 local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
				end
				--重新调用本函数
				print("recall get_ugame_info")
				get_ugame_info(s, msg)
			end)
			return
		end

		--判断status是否正常
		if uinfo.ustatus ~= 0 then 
		    local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.UserIsFreeze
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
		end

		-- 返回给客户端
        local ret_msg = {
					       stype=stype_module.SystemServer,ctype=cmd_module.GetUgameInfoRes,utag=uid,
							body={
									status = res_module.OK,
									uinfo = {
										uchip = uinfo.uchip,
										uexp = uinfo.uexp,
										uvip  = uinfo.uvip,
										uchip2  = uinfo.uchip2,
										uchip3 = uinfo.uchip3, 
										udata1  = uinfo.udata1,
										udata2  = uinfo.udata2,
										udata3 = uinfo.udata3,
								        }
								}
						  }
		utils.print_table(ret_msg)
		session_wrapper.send_msg(s,ret_msg)

   end)

end

local ugame_info = {
	get_ugame_info = get_ugame_info,
}
return ugame_info