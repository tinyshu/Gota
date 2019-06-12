local config = require("conf")
local mysql_conn = nil
local utils = require("utils")

local function is_connectd()
	if not mysql_conn then
	   return false
	end

	return true
end

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

function get_userinfo_by_uid(uid,cb_handle)
	print("get_userinfo_by_uid call")	if mysql_conn==nil then	   cb_handle("mysql is not connect",nil)	   return 	end

	local sql = "select uid, unick, usex, uface, uvip, status ,is_guest from uinfo where uid = %d limit 1";	local sql_cmd = string.format(sql,uid)	print("get_userinfo_by_uid: "..sql_cmd)

	mysql_wrapper.query(mysql_conn,sql_cmd,function(err,t_ret_userinfo)
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
				    cb_handle(nil,t_userinfo)
			end 
	end)

end


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

--处理用户资料编辑请求
function edit_profile_info(uid,unick,uface,usex,cb_handle)
	print("edit_profile_info call")	if mysql_conn==nil then	   if cb_handle then	      cb_handle("mysql is not connect",nil)	   end	   return 	end

	local  sql = "UPDATE uinfo SET unick=\"%s\",uface=%d,usex=%d WHERE uid=%d"
	sql = string.format(sql,unick,uface,usex,uid)
	print(sql)
	mysql_wrapper.query(mysql_conn,sql,function(err,t_ret)
		    if err then
				cb_handle(err,nil)				return 
			end
			cb_handle(nil,nil)

	end)
end

function check_is_guest()
	
end

function check_username_exist(uname,cb_handle)
	print("check_username_exist")
	if string.len(uname)<=0 or mysql_conn == nil then
	   print("uname is empty:")
	   return
	end

	--
	local sql = "SELECT count(*) FROM uinfo WHERE uname=\"%s\""
    local sql_cmd = string.format(sql,uname)
	print(sql_cmd)
	mysql_wrapper.query(mysql_conn,sql_cmd,function(err,t_ret)   
		    if err then
				cb_handle(err,nil)				return 
			end
			
			utils.print_table(t_ret)
			--db里返回的都是表数组，相当于是个二维数组
			--注意底层推送上来的结果value全部是string类型，在lua层做转换
		    local local_ret = t_ret[1]
			print("count:"..local_ret[1])
			cb_handle(nil,tonumber(local_ret[1]))
	end)

end

function do_account_upgrade(uid,uname,upwd,cb_handle)
	
	print("uid"..uid.." uname:"..uname.." ".." upwd:"..upwd)
	local sql = "UPDATE uinfo SET uname=\"%s\" , upwd=\"%s\" , is_guest=0 WHERE uid=%d"
    local sql_cmd = string.format(sql,uname,upwd,uid)
	print(sql_cmd)
	mysql_wrapper.query(mysql_conn,sql_cmd,function(err,t_ret)   
	    if err then
				cb_handle(err,nil)				return 
			end
			print("do_account_upgrade is ok")
			cb_handle(nil,nil)
	end)
end

function get_uinfo_by_uname_upwd(uname, upwd, cb_handler)
    if mysql_conn == nil then 
		  if ret_handler then 
			 cb_handler("mysql is not connected!", nil)
		  end
		  return
	end

	local sql = "select uid, unick, usex, uface, uvip, status, is_guest from uinfo where uname = \"%s\" and upwd = \"%s\" limit 1"
	local sql_cmd = string.format(sql, uname, upwd)	
	print("get_uinfo_by_uname_upwd sql:"..sql_cmd)

	mysql_wrapper.query(mysql_conn,sql_cmd,function(err,ret)   
	    if err then
				cb_handle(err,nil)				return 
			end
			
		
		-- 没有这条记录
		if ret == nil or #ret <= 0 then 
			if cb_handler ~= nil then 
				cb_handler(nil, nil)
			end
			return
		end
		print("get_uinfo_by_uname_upwd return data")
		--查询到数据 24min
		local result = ret[1]
		local uinfo = {}
		uinfo.uid = tonumber(result[1])
		uinfo.unick = result[2]
		uinfo.usex = tonumber(result[3])
		uinfo.uface = tonumber(result[4])
		uinfo.uvip = tonumber(result[5])
		uinfo.status = tonumber(result[6])
		uinfo.is_guest = tonumber(result[7])
		cb_handler(nil, uinfo)

	end)

end

local mysql_auth_center={
	get_guest_user_info = get_guest_user_info,
	insert_guest_user_info = insert_guest_user_info,
	edit_profile_info = edit_profile_info,
	check_username_exist = check_username_exist,
	get_userinfo_by_uid = get_userinfo_by_uid,
	do_account_upgrade = do_account_upgrade,
	get_uinfo_by_uname_upwd = get_uinfo_by_uname_upwd,
	is_connectd = is_connectd,
}

return mysql_auth_center