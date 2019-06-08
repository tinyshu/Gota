--定义协议返回码

local ResponesCode = {
	OK = 1,
	SystemErr = -100,
	InvaildErr = -101,
	--认证中心
	UserIsFreeze = -1000,
	UserIsNotGuest = -1001,
	UserRelogin = -1002,
	UserNotFound = -1003,
	UserNameExist = -1004,
	UserIsNotGuest = -1005,
} 

return ResponesCode