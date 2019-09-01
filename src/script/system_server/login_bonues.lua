--每日奖励模块
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_game = require("database/mysql_system_ugame")
local utils = require("utils")
local ugame_info_conf = require("user_game_config")

--判断是否需要发放奖励 36min
local function send_bonues_to_user(uid, bonues_info, ret_handler)
   --每天能领取一次奖励(判断规则: 上次发放时间戳是否<当天的时间抽 成立就发放)
   --连续登录的判断，当天登录时间戳>昨天时间抽 && <今天时间抽
   	if bonues_info.bonues_time < utils_wrapper.timestamp_today() then
		if bonues_info.bonues_time >= utils_wrapper.timestamp_yesterday() then -- 连续登陆
			bonues_info.days = bonues_info.days + 1
		else 
		    --重新开始计算
			bonues_info.days = 1
		end
			 
		if bonues_info.days > #ugame_info_conf.login_bonues then 
		    --重新开始计算
			bonues_info.days = 1
		end

		--设置为发放奖励状态
		bonues_info.status = 0
		--发放奖励时间
		bonues_info.bonues_time = utils_wrapper.timestamp()
		--设置当天领取奖励个数
		bonues_info.bonues = ugame_info_conf.login_bonues[bonues_info.days]
		--更新用户当日奖励db数据
	    mysql_game.update_login_bonues(uid, bonues_info, function (err, ret)
			if err then 
				ret_handler(err, nil)
				return
			end

			   ret_handler(nil, bonues_info)
		end)
		
		return
    end

	-- 把登陆奖励信息返回给ugame
	ret_handler(nil, bonues_info)
end
--给回调cb_handle(err,bonues_info)
function ckeck_login_bonues(uid,cb_handle)
	print("ckeck_login_bonues")
	if uid <= 0 then
	  if cb_handle~=nil then
	     cb_handle("uid is err",nil)     
	  end
	  return
	end

	mysql_game.get_bonues_info(uid, function (err, bonues_info)
	  if err then
			cb_handle(err, nil)
			return 
		end

		-- 这个用户第一次来登陆，默认给插入一条记录
	  if bonues_info == nil then
	     	    mysql_game.insert_bonues_info(uid, function (err, ret)
				if err ~= nil then
				    print("insert_bonues_info"..err)
					cb_handle(err, nil)
					return
				end
				--在此调用自己，来拉取奖励数据
				ckeck_login_bonues(uid, cb_handle)
			end)
			return 
	  end

	  --发送奖励信息给用户
	  send_bonues_to_user(uid, bonues_info, cb_handle)
	end)
	
end

function recv_login_bonues(s,msg)
	uid = msg[3]
	print("recv_login_bonues uid:"..uid)
	mysql_game.get_bonues_info(uid, function (err, bonues_info)
	  if err then
		 --返回系统错误
		  local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
	  end

		-- 这个用户第一次来登陆，默认给插入一条记录
	  if bonues_info == nil or bonues_info.status~=0 then
	      local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.InvalidOptErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
	  end

	  --可以领取
	  mysql_game.update_login_bonues_status(uid,function(err,ret)
	        if err ~= nil then
	        	 local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			        session_wrapper.send_msg(s,ret_msg)
					return
			end
            
			--更新金币数据
			mysql_game.add_chip(uid, bonues_info.bonues, nil)
			local ret_msg = {
					        stype=stype_module.SystemServer,ctype=cmd_module.RecvLoginBonuesRes,utag=uid,
							body={
									status = res_module.OK
							}}

			 session_wrapper.send_msg(s,ret_msg)

      end)

	end)

end

local login_bonues = {
	ckeck_login_bonues = ckeck_login_bonues,
	recv_login_bonues = recv_login_bonues,
}
return login_bonues