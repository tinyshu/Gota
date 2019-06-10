--改文件定义用于底层根据名字反射出protobuf的message对象
--协议的位置需要和在proto定义的id一样
--比如GuestLoginReq在proto里定义的协议id是1，这里就要放第一个
--这里的名称是message的协议名称,需要完全相同
local cmd_name_map = {
	"GuestLoginReq",
	"GuestLoginRes",
	"ReLoginRes",
	"UserLostConn",
	"EditProfileReq",
	"EditProfileRes",
	"AccountUpgradeReq",
	"AccountUpgradeRes",
	"UnameLoginReq",
	"UnameLoginRes",
	"LoginOutReq",
	"LoginOutRes",
	"GetUgameInfoReq",
	"GetUgameInfoRes",
	"RecvLoginBonuesReq",
	"RecvLoginBonuesRes"
}

return cmd_name_map
