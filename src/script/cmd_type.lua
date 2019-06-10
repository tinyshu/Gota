--和proto文件里定义的协议cmd对应 ,用在消息返回使用协议id应用
--提供给lua层使用的宏定义，需要和proto文件里定义的完全一样
local Cmd = {
	GuestLoginReq = 1,
	GuestLoginRes = 2,
	ReLoginRes = 3,
	UserLostConn =4,
	EditProfileReq = 5,
	EditProfileRes = 6,
	AccountUpgradeReq = 7,
	AccountUpgradeRes = 8,
	UnameLoginReq = 9,
	UnameLoginRes = 10,
	LoginOutReq = 11,
	LoginOutRes = 12,
	GetUgameInfoReq = 13,
	GetUgameInfoRes = 14,
	RecvLoginBonuesReq = 15,
	RecvLoginBonuesRes = 16
}
     
return Cmd