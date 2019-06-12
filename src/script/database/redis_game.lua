--game信息redis操作模块
local config = require("conf")

local redis_conn = nil

local function is_connectd()
	if not redis_conn then
	   return false
	end

	return true
end

function redis_connect_to_game()
	local host = config.game_redis.host
	local port = config.game_redis.port
	local db_index = config.game_redis.db_index
	local pwd = config.pwd

	redis_wrapper.connect(host, port,pwd,5000, function (err, conn)
		if err ~= nil then
			print(err)
			timer_wrapper.create_timer_once(redis_connect_to_game,5000,5000)
			return
		end

		redis_conn = conn
		print("connect to redis game db success!!!!")
		redis_wrapper.query(redis_conn,"SELECT "..db_index,function(err,ret)
	        if err ~= nil then
			   print("select redis error :"..err)
			end
       end)
	end)

end

redis_connect_to_game()

function set_ugame_info_inredis(uid, ugame_info)
	if redis_conn == nil then 
		print("redis game disconnected")
		return
	end

	local redis_cmd = "hmset moba_ugame_user_uid_" .. uid ..
	                  " uchip " .. ugame_info.uchip ..
	                  " uexp " .. ugame_info.uexp .. 
	                  " uvip " .. ugame_info.uvip
	print("redis_cmd: "..redis_cmd)
	redis_wrapper.query(redis_conn, redis_cmd, function (err, ret)
		if err then
		    print("set_ugame_info_inredis"..err)
			return 
		end
	end)
end

-- ret_handler(err, uinfo)
function get_ugame_info_inredis(uid, ret_handler)
	if redis_conn == nil then 
		print("redis game disconnected")
		return
	end

	local redis_cmd = "hgetall moba_ugame_user_uid_" .. uid
	redis_wrapper.query(redis_conn, redis_cmd, function (err, ret)
		if err then
			if ret_handler then 
				ret_handler(err, nil)
			end
			return 
		end

		local ugame_info = {}
		ugame_info.uid = uid
		ugame_info.uchip = tonumber(ret[2])
		ugame_info.uexp = tonumber(ret[4])
		ugame_info.uvip = tonumber(ret[6])

		ret_handler(nil, ugame_info)		
	end)
end

function add_chip_inredis(uid, add_chip)
	if redis_conn == nil then 
		print("redis game disconnected")
		return
	end
	
	get_ugame_info_inredis(uid, function (err, ugame_info)
		if err then
			print("get ugame_info inredis error")
			return 
		end

		ugame_info.uchip = ugame_info.uchip + add_chip

		set_ugame_info_inredis(uid, ugame_info)
	end)
end

local redis_game ={
	is_connectd = is_connectd,
	redis_connect_to_game = redis_connect_to_game,
	set_ugame_info_inredis = set_ugame_info_inredis,
	get_ugame_info_inredis = get_ugame_info_inredis,
	add_chip_inredis = add_chip_inredis,

}

return redis_game