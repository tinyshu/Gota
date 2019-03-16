--和proto文件里定义的协议cmd对应 ,用在消息返回使用协议id应用
local Cmd = {
	GuestLoginReq = 1,
	GuestLoginRes = 2,
	ReLoginRes = 3,
	UserLostConn =4,
}

return Cmd