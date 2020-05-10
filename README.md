

####  主要特性

- 跨平台的服务器框架

- 网络层使用高效的libuv库

- 事件驱动模型,单进程异步方式

- 支持异步mysql

- 支持异步redis

- 支持内嵌lua，导出底层接口，上层业务使用lua开发

- 网络层支持TCP UDP WebSocket协议

- 应用层多协议支持(protobuf ,json)

- 支持部署和调试两种模式

   部署模式：服务集群之间通过netbus模块转发消息

   开发调试模式：整个集群的逻辑都可以放入一个进程，如gatawayserver开发调试

   能极大提高开发调试效率

- 支持lua配置文件

​ 

####  配置和启动

#####     目录结构

​     3rd：第三方依赖库

​     database: db读写的封装

​     devlop:  废弃

​     export_module:导出给lua层接口

​     lua_wrapper:lua层封装

​     moduel:封装网络底层和session管理 

​     proto:存放协议目录

​     script:存储应用层lua脚本，下面建了4个服务器(这个可以根据自己业务创建),如何新建一个服务可以参考该服务下的服务 

utils:公共代码

proj.win: vs2015工程配置相关文件

tools: 工具相关

sql:

##### 服务启动和相关配置

**db相关配置:**

1.在script目录下conf.lua文件里配置你的db,redis，服务启动的ip，port相关信息，其他配置都可以放入这个文件。

​     local game_config = {
	gateway_tcp_ip = "127.0.0.1",
	gateway_tcp_port = 6080,

	gateway_ws_ip = "127.0.0.1",
	gateway_ws_port = 6081,
	
	servers = remote_servers,
	
	auth_mysql = {
		host = "mydb_ip", -- 数据库所在的host
		port = 3306,        -- 数据库所在的端口
		db_name = "auth_center",  -- 数据库的名字
		uname = "root",      -- 登陆数据库的账号
		upwd = "tiny550122ABC",     -- 登陆数据库的密码
	},
	
	center_redis = {
		host = "myredis_ip", -- redis所在的host
		port = 6379, -- reidis 端口
		db_index = 1, -- 数据1
	}, 
	
	moba_mysql = {
		host = "mydb_ip", -- 数据库所在的host(写上自己的ip)
		port = 3306,        -- 数据库所在的端口
		db_name = "moba_game",  -- 数据库的名字
		uname = "root",      -- 登陆数据库的账号
		upwd = "tiny550122ABC",     -- 登陆数据库的密码
	},
	
	game_redis = {
		host = "myredis_ip", -- redis所在的host
		port = 6379, -- reidis 端口
		db_index = 2, -- 数据库2
	},
	
	logic_upd = {
		host = "127.0.0.1",    //逻辑服务upd监听端口
		port = 8800,
	},

}

2.一个服务进程配置如下：

​        remote_servers[stype.AuthSerser] = {
	stype = stype.AuthSerser,    //服务id,用户服务消息转发
	ip = "127.0.0.1",                     //服务ip和端口
	port = 8000,
	desic = "Auth server",          //描述
}

**启动方式:**

bin目录下直接启动: 点击start_all.bat或者点击单独的bat文件

调试方式: 一般会单独调试一个服务器，比如在开发阶段需要单独调试logic_server步骤
                 1.先启动其他需要启动的服务(比如当前的是start_auth，start_gateway，start_system)，

​                  2.在IDE中修改main.cpp文件

​                  3.然后直接在vs2015里调试状态下启动即可

![1](.\images\1.png)



 **调试log**                  

如果是直接运行log在bin目录下的 logger目录里

如果是调试方式，log在src的目录里

####    学习和使用指南

​**  如何增加一个协议**

添加新协议的规则
1.在game.proto文件添加新协议和结构
    enum Cmd添加协议id
	添加新协议和结构
2.在cmd_type添加新协议的id
3.在cmd_name_map.lua添加命令名字,服务引擎在启动
4.运行proto文件夹下的bat文件，重新生成新协议文件
5.重新编译服务引擎
*/

#### 联系方式

* qq: 2690540630