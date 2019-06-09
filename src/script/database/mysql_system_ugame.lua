--对moba_game db的相关操作
local config = require("conf")
local utils = require("utils")
local ugame_info_conf = require("user_game_config")

local mysql_conn = nil

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

local mysql_game = {
	get_ugame_info = get_ugame_info,
	insert_ugame_info = insert_ugame_info,
}

return mysql_game