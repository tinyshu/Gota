local config = require("conf")
local mysql_conn = nil

function mysql_connect_auth_center()

	local auth_conf = config.auth_mysql
	mysql_wrapper.connect(auth_conf.host,auth_conf.port,auth_conf.db_name,
							auth_conf.uname,auth_conf.upwd,function(err, conn)
				if err ~= nil then
				--Logger_wrapper.
					print(err)
					--重新连接
					timer_wrapper.create_timer_once(check_session_connect,5000,5000)
					return
				end
				print("connect auth db sucess!!")
				mysql_conn = conn
    end)
end
--脚本被加载就调用该函数
mysql_connect_auth_center()

function get_guest_user_info(guest_key,cb_handle)
    
	local sql = "select uid, unick, usex, uface, uvip, status ,is_guest from uinfo where guest_key = \"%s\" limit 1";	local sql_cmd = string.format(sql,guest_key)	print(sql_cmd)	if mysql_conn==nil then	   cb_handle("mysql is not connect",nil)	   return 	end	mysql_wrapper.query(mysql_conn,sql_cmd,function(err,t_ret_userinfo)
			--查询错
			if err then
				cb_handle(err,nil)				return 
			end

			--没有找到数据
			if t_ret_userinfo==nil or #t_ret_userinfo<=0 then
				if cb_handle~=nil then
				    cb_handle(nil,nil)
				end
			    --跳出回调函数				return 
			end

			--查询到数据
			local result = t_ret_userinfo[1]
			if cb_handle~=nil then
			        --返回一个表给回调函数
			        local t_userinfo = {}
					t_userinfo.uid = tonumber(result[1])
					t_userinfo.unick = result[2]
					t_userinfo.usex = tonumber(result[3])
					t_userinfo.uface = tonumber(result[4])
					t_userinfo.uvip = result[5]
					t_userinfo.status = tonumber(result[6])
					t_userinfo.is_guest = tonumber(result[7])
					print_r(t_userinfo)
				    cb_handle(nil,t_userinfo)
			end 
	end)	
end

function insert_guest_user_info(guest_key, cb_handle)
	print("insert_guest_user_info call")	if mysql_conn==nil then	   cb_handle("mysql is not connect",nil)	   return 	end
	local sql =  "insert into uinfo(`guest_key`, `unick`, `uface`, `usex`, `is_guest`,`status`)values(\"%s\", \"%s\", %d, %d, 1,0)";	local unick = "guest"..math.random(100000,999999)

	local ufance = math.random(1,9)
	local sex = math.random(0,1)

	local sql_cmd = string.format(sql,guest_key,unick,ufance,sex)
	mysql_wrapper.query(mysql_conn,sql_cmd,function(err,t_ret)
	      --查询错误
			if err then
				cb_handle(err,nil)				return 
			end
			cb_handle(nil,nil)
    end)
	
end

local mysql_auth_center={
	get_guest_user_info = get_guest_user_info,
	insert_guest_user_info = insert_guest_user_info,
}

return mysql_auth_center