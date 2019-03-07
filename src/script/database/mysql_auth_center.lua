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
--mysql_connect_auth_center()