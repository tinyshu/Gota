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
	UserNameExist = -1004,    --游客升级用户名称已经存在
	UserIsNotGuest = -1005,   --不是游客状态
	UserUnamePwdErr = -1006,  --正式登录用户名或密码错误
	InvalidOptErr   = -1007,  --非法操作
} 

return ResponesCode