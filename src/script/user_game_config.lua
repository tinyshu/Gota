--定义系统服务器在用户第一次初始化设置默认值
local user_game_config = {
	ugame = {
		uchip = 2000,
		uvip = 0,
		uexp = 0,
		-- ...
	},	

	--奖励配置: 连续登录5天在重100开始
	login_bonues = {100, 200, 300, 400, 500},
}


return user_game_config


