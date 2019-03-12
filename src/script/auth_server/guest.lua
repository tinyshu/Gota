--游客模块逻辑
---- {stype, ctype, utag, body}
--游客登录

--加载模块时候连接认证服务器
--17min
mysql_center = require("database/mysql_auth_center")
utils = require("utils")

function guest_login(s,msg)
	print(msg[4].guest_key)
	local guest_key = msg[4].guest_key

	--db层接口返回的数据是lua的table形式
	mysql_center.get_guest_user_info(guest_key,function(err,user_info)
		if err then
		--返回给客户端错误消息
		  print(err)
		  return 
		end

		--判断是否是否存在
		if user_info==nil then
		  --没有数据请求插入数据
		    mysql_center.insert_guest_user_info(guest_key,function (err,ret)
					if err then
						--返回给客户端错误消息
						print(err)
						return 
					 end
					 --插入成功，重新调用自己
					 guest_login(s,msg)
            end)
			return 
		end
		--这里查到了，先判断状态
		--print_r(user_info)
		utils.print_table(user_info)
		if user_info.status ~= 0 then
		     --状态不正确 ,返回信息给客户端
			 print("user status error status:"..user_info.status)
		     return
		end
		--判断用户是否为游客状态
		if user_info.is_guest ~=1 then
		    --不是游客状态无法使用游客key登录
			print("user is_guest error"..user_info.is_guest)
			return 
		end
		print("user data"..user_info.uid,user_info.unick,user_info.status)
		--返回登录成功消息给客户端
    end)
end

local guest = {
	login = guest_login
}

return guest

