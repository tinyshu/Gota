local config = require("conf")
local redis_conn = nil

function redis_connect_auth_center()
	local redis_auth_conf = config.center_redis
	local host = redis_auth_conf.host
	local port = redis_auth_conf.port
	local db_index = redis_auth_conf.db_index
	redis_wrapper.connect(host,port,2000,function(err,conn)
	  if err ~= nil or conn==nil then
	     print("connect redis error :"..err)
		 --重新连接
		 timer_wrapper.create_timer_once(redis_connect_auth_center,5000,5000)
		 return
	  end

	   print("connect redis sucess!")
	   redis_conn = conn
	   redis_wrapper.query("SELECT "..db_index,function(err,ret)
	        if err ~= nil then
			   print("select redis error :"..err)
			end
       end)
    end)

end

redis_connect_auth_center()

function set_userinfo_to_redis(uid,uinfo)
	
end

function get_userinfo_to_redis(err,uinfo)
	
end

function edit_profile_to_redis(uid,unkci,uface,usex)
	
end

local redis_auth_center={
	set_userinfo_to_redis = set_userinfo_to_redis,
    get_userinfo_to_redis = get_userinfo_to_redis,
	edit_profile_to_redis = edit_profile_to_redis
}

return mysql_auth_center