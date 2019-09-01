--正式登录模块
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local utils = require("utils")

function uname_login_process(s,msg)
    --这里还没有登录，没有uid,只有utag
	--utag是网关分配的临时session id
	local cutag = msg[3]
	print("uname_login_process: utag:"..cutag)
	local unamelogin_req = msg[4]
	if string.len(unamelogin_req.uname) <=0 or string.len(unamelogin_req.upwd_md5) ~=32 then
	  local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.InvaildErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
	end

	--参数正确，查询db数据，判断用户名，密码是否正确
	print("uname:"..unamelogin_req.uname.." ".."upwd:"..unamelogin_req.upwd_md5)
	mysql_center.get_uinfo_by_uname_upwd(unamelogin_req.uname,unamelogin_req.upwd_md5,function(err,userinfo)
	     if err ~= nil then
		    local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.SystemErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		 end

		 --没有查询到用户密码错误
		 if userinfo == nil then 
		  local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.UserNotFound
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		end

		--判断帐号状态
		if userinfo.status ~= 0 then
		    local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.UserIsFreeze
							}}

			session_wrapper.send_msg(s,ret_msg)
			return
		end

		--正式登录成功
		
		redis_center.set_userinfo_to_redis(userinfo.uid,userinfo)
		--杩斿洖鐧诲綍鎴愬姛娑堟伅缁欏鎴风
		local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.UnameLoginRes,utag=cutag,
							body={
									status = res_module.OK,
									userinfo = {
									   --这里的key需要和pb协议名字一样，或者在底层
									   --做反序列化pb找不到对应字段
									   unick = userinfo.unick,
									   uface = userinfo.uface,
									   usex = userinfo.usex,
									   uvip = userinfo.uvip,
									   uid = userinfo.uid,
									}
							}}
		utils.print_table(ret_msg)
		session_wrapper.send_msg(s,ret_msg)
		print("uname_login_process return data")
		
    end)
end

local unamelogin = {
	uname_login_process = uname_login_process,
}

return unamelogin