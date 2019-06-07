local config = require("conf")
local redis_conn = nil
--user key =auth_center_user_uid_xxxx形式 
auth_key = "auth_center_user_uid_" 

function redis_connect_auth_center()
	local redis_auth_conf = config.center_redis
	local host = redis_auth_conf.host
	local port = redis_auth_conf.port
	local db_index = redis_auth_conf.db_index
	local pwd = redis_auth_conf.pwd
	redis_wrapper.connect(host,port,pwd,5000,function(err,conn)
	  if err ~= nil or conn==nil then
	     print("connect redis error :"..err)
		 --重新连接
		 timer_wrapper.create_timer_once(redis_connect_auth_center,5000,5000)
		 return
	  end

	   print("connect redis sucess!")
	   redis_conn = conn
	   redis_wrapper.query(redis_conn,"SELECT "..db_index,function(err,ret)
	        if err ~= nil then
			   print("select redis error :"..err)
			end
       end)
    end)

end

redis_connect_auth_center()

function set_userinfo_to_redis(uid,uinfo)
	print("set_userinfo_to_redis uid:"..uid)
	if uid < 0 or redis_conn== nil then
	   print("uid error uid:"..uid)
	   return
	end
	--使用hmset命令
	--HMSET key field value [field value …] 给hash设置值
	--hmset key unkci v1 uface v2 usezx v3
	local key = auth_key .. uid 
	print("key:"..key)
	local redis_cmd = "hmset " .. key .. 
	" unick " .. uinfo.unick ..
	" uface " .. uinfo.uface ..
	" usex "  .. uinfo.usex ..
	" uvip "  .. uinfo.uvip ..
	" is_guest ".. uinfo.is_guest
	print(redis_cmd)
	redis_wrapper.query(redis_conn,redis_cmd,function(err,ret)
	  if err then
	     print("err"..err)
	  end
    end)
end

function get_userinfo_to_redis(uid,cb_handle)
	print("get_userinfo_to_redis uid:"..uid)
	if uid < 0 or redis_conn == nil then
	   print("uid error uid:"..uid)
	   return
	end

   local key = auth_key .. uid
   --hgetall获取字段key全部数据，底层数据作为array返回 
   local redis_cmd = "hgetall " .. key
   print(redis_cmd)
   redis_wrapper.query(redis_conn,redis_cmd,function(err,ret)
	  if err then
	    if cb_handle ~= nil then
		   cb_handle(err,nil)
		end
	  end

	  --正确获取数据
	  local uinfo = {}
	  uinfo.uid = uid
	  uinfo.unick = ret[2]
	  uinfo.uface = tonumber(ret[4])
	  uinfo.usex = tonumber(ret[6])
	  uinfo.uvip = tonumber(ret[8])
	  uinfo.is_guest = tonumber(ret[10])
	  cb_handle(nil,uinfo)
    end)
end

function edit_profile_to_redis(uid,unick,uface,usex)
	print("edit_profile_to_redis uid:"..uid)
	if uid <= 0 or redis_conn == nil then
	   print("uid error uid:"..uid)
	   return
	end

	get_userinfo_to_redis(uid,function(err,ret)
	    print("set_userinfo_to_redis by edit AAA!")
	    if err ~= nil then
		   print("get_userinfo_to_redis err:"..err)
		   return
		end

		ret.unick = unick
		ret.uface = uface 
		ret.usex = usex
		print("set_userinfo_to_redis by edit BBB!")
		set_userinfo_to_redis(uid,ret)
    end)
end

local redis_auth_center={
	set_userinfo_to_redis = set_userinfo_to_redis,
    get_userinfo_to_redis = get_userinfo_to_redis,
	edit_profile_to_redis = edit_profile_to_redis
}

return redis_auth_center