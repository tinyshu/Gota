--定义协议返回码

local ResponesCode = {
	OK = 0,
	SystemErr = -100,
	--认证中心
	UserIsFreeze = -1000,
	UserIsNotGuest = -1001,
	UserRelogin = -1002,
} 

return ResponesCode