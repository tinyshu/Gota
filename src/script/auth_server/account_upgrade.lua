--游客帐号升级模块
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local utils = require("utils")

--更新db
function _do_account_upgrade(s,msg)
    local uid = msg[3]
	print("_do_account_upgrade uid:"..uid)
	mysql_center.get_userinfo_by_uid(uid,function(err,ret)
	     if err ~= nil then
		     local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			session_wrapper.send_msg(s,ret_msg)
	        return   
		 end

		 local user_info = ret
		 if user_info.is_guest ~= 1 then
		    --不是游客，无法升级
			print("user is not guest!! uid:"..uid)
			 local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.UserIsNotGuest
							}}

			session_wrapper.send_msg(s,ret_msg)
	        return   
		 end

		 --update database		
		 local uname = msg[4].uname
		 local upwd =  msg[4].upwd_md5
		 mysql_center.do_account_upgrade(uid,uname,upwd,function(err,ret)
	           if err ~= nil then
		          local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			      session_wrapper.send_msg(s,ret_msg)
	              return   
		      end
			  --更新执行成功，返回消息
			   local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.OK
							}}

			  session_wrapper.send_msg(s,ret_msg)
         end)

    end)
end

function account_upgrade_process(s,msg)
	local uid = msg[3]
	print("account_upgrade uid:"..uid)
	
	if uid <=0 then
	   print("account_upgrade uid is error!")
	   return 
	end

	local account_upgrade_req = msg[4]
	if string.len(account_upgrade_req.uname)<=0 or 
	   string.len(account_upgrade_req.upwd_md5) ~= 32 then
	    local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.InvaildErr
							}}

			session_wrapper.send_msg(s,ret_msg)
	   return   
	end

	--参数检查正确
	print("uname:"..account_upgrade_req.uname," upwd_md5:"..account_upgrade_req.upwd_md5)
	--检查uid是不是游客状态
	--检查uname是否存在
	   mysql_center.check_username_exist(account_upgrade_req.uname,function(err,ret)
	   if err ~= nil then
	       local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.SystemErr
							}}

			session_wrapper.send_msg(s,ret_msg)
	        return   
	   end

	   --判断是否有重名uname
	   if ret > 0 then
	       local ret_msg = {
					        stype=stype_module.AuthSerser,ctype=cmd_module.AccountUpgradeRes,utag=uid,
							body={
									status = res_module.UserNameExist
							}}

			session_wrapper.send_msg(s,ret_msg)
	        return   
	   end

	   _do_account_upgrade(s,msg)
    end)
end


local accountupgrade = {
	account_upgrade_process = account_upgrade_process,
}

return accountupgrade