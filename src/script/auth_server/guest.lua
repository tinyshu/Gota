--游客模块逻辑
---- {stype, ctype, utag, body}
--游客登录
--14min
--加载模块时候连接认证服务器 --20min
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_center = require("database/mysql_auth_center")
local redis_center = require("database/redis_auth_center")
local utils = require("utils")

--guest login process
function guest_login(s,msg)
	print(msg[4].guest_key)
	local guest_key = msg[4].guest_key

	--db层接口返回的数据是lua的table形式
	mysql_center.get_guest_user_info(guest_key,function(err,user_info)
		if err then
		--返回给客户端错误消息
		  print(err)
		  --local ret_msg = {stype=stype.AuthSerser,ctype=2,utag=msg[3],body={status=200}}
		  local ret_msg = {
					  stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
					  body={
					      status = res_module.UserNotFound
					    }}

					  session_wrapper.send_msg(s,ret_msg)
			return 
		
		end

		--判断是否是否存在
		if user_info==nil then
		  --没有数据请求插入数据
		    mysql_center.insert_guest_user_info(guest_key,function (err,ret)
					if err then
						--返回给客户端错误消息
						print(err)
						local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.SystemErr
							}}

						session_wrapper.send_msg(s,ret_msg)
						return 
					 end
					 --插入成功，重新调用自�?
					 guest_login(s,msg)
            end)
			return 
		end
		--这里查到了，先判断状�?
		--utils.print_table(user_info)
		if user_info.status ~= 0 then
		     --状态不正确 ,返回信息给客户端
			 print("user status error status:"..user_info.status)
			 local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.UserIsFreeze
							}}

			session_wrapper.send_msg(s,ret_msg)
		     return
		end
		--判断用户是否为游客状�?
		if user_info.is_guest ~=1 then
		    --不是游客状态无法使用游客key登录
			print("user is_guest error"..user_info.is_guest)
			local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.UserIsNotGuest
							}}

			session_wrapper.send_msg(s,ret_msg)

			return 
		end
		print("guest_login user data"..user_info.uid,user_info.unick,user_info.status)
		--��¼�ɹ����û���Ϣ�洢��
		redis_center.set_userinfo_to_redis(user_info.uid,user_info)
		--返回登录成功消息给客户端
		local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.GuestLoginRes,utag=msg[3],
							body={
									status = res_module.OK,
									userinfo = {
									   --�����key��Ҫ��pbЭ������һ���������ڵײ�
									   --�������л�pb�Ҳ�����Ӧ�ֶ�
									   unick=user_info.unick,
									   uface=user_info.uface,
									   usex=user_info.usex,
									   uvip=user_info.uvip,
									   uid=user_info.uid,
									}
							}}
		utils.print_table(ret_msg)
		session_wrapper.send_msg(s,ret_msg)

    end)
end

local guest = {
	login = guest_login
}

return guest

