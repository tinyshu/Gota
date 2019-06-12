--对moba_game db的相关操作
local config = require("conf")
local utils = require("utils")
local ugame_info_conf = require("user_game_config")

local mysql_conn = nil

local function is_connectd()
	if not mysql_conn then
	   return false
	end

	return true
end

function mysql_connect_to_moba_game()
      local moba_conf = config.moba_mysql
	  mysql_wrapper.connect(moba_conf.host,moba_conf.port,moba_conf.db_name,
							moba_conf.uname,moba_conf.upwd,function(err, conn)
				if err ~= nil then
					print(err)
					--重新连接
					timer_wrapper.create_timer_once(mysql_connect_to_moba_game,5000,5000)
					return
				end
				print("connect system moba db sucess!!")
				mysql_conn = conn
    end)

end

mysql_connect_to_moba_game()

function get_ugame_info(uid, ret_handler)
    if mysql_conn == nil or uid <=0 then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end
	
	local sql = "select uchip, uchip2, uchip3, uvip, uvip_endtime, udata1, udata2, udata3, uexp, ustatus from ugame where uid = %d limit 1"
	local sql_cmd = string.format(sql, uid)	
	print(sql_cmd)

	mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
	    
		if err then 
			if ret_handler ~= nil then 
				ret_handler(err, nil)
			end
			return
		end

		-- 没有这条记录
		if ret == nil or #ret <= 0 then 
			if ret_handler ~= nil then 
				ret_handler(nil, nil)
			end
			return
		end

		local result = ret[1]
		local uinfo = {}
		uinfo.uchip = tonumber(result[1])
		uinfo.uchip2 = tonumber(result[2])
		uinfo.uchip3 = tonumber(result[3])
		uinfo.uvip = tonumber(result[4])
		uinfo.uvip_endtime = tonumber(result[5])
		uinfo.udata1 = tonumber(result[6])
		uinfo.udata2 = tonumber(result[7])
		uinfo.udata3 = tonumber(result[8])
		uinfo.uexp = tonumber(result[9])
		uinfo.ustatus = tonumber(result[10])

		ret_handler(nil, uinfo)

	end)
end

function insert_ugame_info(uid, ret_handler)
    if mysql_conn == nil or uid <=0 then 
		if ret_handler then 
			ret_handler("insert_ugame_info error!", nil)
		end
		return
	end

	
	local sql = "insert into ugame(`uid`, `uchip`, `uvip`, `uexp`)values(%d, %d, %d, %d)"
	local sql_cmd = string.format(sql, uid, 
		            ugame_info_conf.ugame.uchip, 
		            ugame_info_conf.ugame.uvip, 
		            ugame_info_conf.ugame.uexp)

    mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
	     	if err then 
			   ret_handler(err, nil)
			   return
		    else
			ret_handler(nil, nil)
		end
	end)

end

function get_bonues_info(uid, ret_handler)
   if mysql_conn == nil then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end

	local sql = "select bonues, status, bonues_time, days from login_bonues where uid = %d limit 1"
	local sql_cmd = string.format(sql, uid)	
	print(sql_cmd)

	
	mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
		if err then 
			if ret_handler ~= nil then 
				ret_handler(err, nil)
			end
			return
		end

		-- 没有这条记录
		if ret == nil or #ret <= 0 then 
			if ret_handler ~= nil then 
				ret_handler(nil, nil)
			end
			return
		end
		-- end
		
		local result = ret[1]
		local bonues_info = {}
		bonues_info.bonues = tonumber(result[1])
		bonues_info.status = tonumber(result[2])
		bonues_info.bonues_time = tonumber(result[3])
		bonues_info.days = tonumber(result[4])
		
		ret_handler(nil, bonues_info)
	end)

end

function insert_bonues_info(uid, ret_handler)
	if mysql_conn == nil then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end

	local sql = "INSERT INTO login_bonues(`uid`, `bonues_time`, `status`)VALUES(%d, %d, 1)"
	local sql_cmd = string.format(sql, uid, utils_wrapper.timestamp())
	print(sql_cmd)
	mysql_wrapper.query(mysql_conn, sql_cmd, function (err, ret)
		if err then 
			ret_handler(err, nil)
			return
		else
			ret_handler(nil, nil)
		end
	end)
end

function update_login_bonues(uid, bonues_info, ret_handler)
    if mysql_conn == nil then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end

	local sql = "update login_bonues set status = 0, bonues = %d, bonues_time = %d, days = %d where uid = %d"
	local sql_cmd = string.format(sql, bonues_info.bonues, bonues_info.bonues_time, bonues_info.days, uid)
	
	mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
		if err then
			if ret_handler ~= nil then 
				ret_handler(err, nil)
			end
			return
		end

		if ret_handler then 
			ret_handler(nil, nil)
		end
	end)
end


function update_login_bonues_status(uid, ret_handler) 
	if mysql_conn == nil then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end

	local sql = "update login_bonues set status = 1 where uid = %d"
	local sql_cmd = string.format(sql, uid)
	
	mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
		if err then
			if ret_handler ~= nil then 
				ret_handler(err, nil)
			end
			return
		end

		if ret_handler then 
			ret_handler(nil, nil)
		end
	end)
end

function  add_chip(uid, chip, ret_handler)
	if mysql_conn == nil then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end

	local sql = "update ugame set uchip = uchip + %d where uid = %d"
	local sql_cmd = string.format(sql, chip, uid)
	
	mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
		if err then
			if ret_handler ~= nil then 
				ret_handler(err, nil)
			end
			return
		end

		if ret_handler then 
			ret_handler(nil, nil)
		end
	end)
end

function get_robots_ugame_info(ret_handler)
	if mysql_conn == nil then 
		if ret_handler then 
			ret_handler("mysql is not connected!", nil)
		end
		return
	end

	local sql = "select uchip, uchip2, uchip3, uvip, uvip_endtime, udata1, udata2, udata3, uexp, ustatus, uid from ugame where is_robot = 1"
	local sql_cmd = sql
	print(sql_cmd)
	mysql_wrapper.query(mysql_conn, sql_cmd, function(err, ret)
		if err then 
			if ret_handler ~= nil then 
				ret_handler(err, nil)
			end
			return
		end

		-- 没有这条记录
		if ret == nil or #ret <= 0 then 
			if ret_handler ~= nil then 
				ret_handler(nil, nil)
			end
			return
		end
		-- end
		
		local k, v
		local robots = {}
		for k, v in pairs(ret) do 
			local result = v
			local one_robot = {}
			one_robot.uchip = tonumber(result[1])
			one_robot.uchip2 = tonumber(result[2])
			one_robot.uchip3 = tonumber(result[3])
			one_robot.uvip = tonumber(result[4])
			one_robot.uvip_endtime = tonumber(result[5])
			one_robot.udata1 = tonumber(result[6])
			one_robot.udata2 = tonumber(result[7])
			one_robot.udata3 = tonumber(result[8])
			one_robot.uexp = tonumber(result[9])
			one_robot.ustatus = tonumber(result[10])
			one_robot.uid = tonumber(result[11])

			table.insert(robots, one_robot)
		end

		ret_handler(nil, robots)
	end)
end

local mysql_game = {
	get_ugame_info = get_ugame_info,
	insert_ugame_info = insert_ugame_info,
	get_bonues_info = get_bonues_info,
	insert_bonues_info = insert_bonues_info,
	update_login_bonues = update_login_bonues,
	update_login_bonues_status = update_login_bonues_status,
	add_chip = add_chip,
	is_connectd = is_connectd,
	get_robots_ugame_info = get_robots_ugame_info,
}

return mysql_game