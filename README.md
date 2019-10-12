**Goat服务器通用引擎**

####  主要特性


框架特点:
全异步单进程架构
    底层全部处理,网络收发，mysql读写，redis读写，写log文件全部使用高效的异步机制，并且给业务层完全屏蔽，做到高效简单

跨平台:
      网络底层使用高性能libuv网络库(没听过? 那node.js你一定知道)
      使用尽量使用全部C++或者std标准库，如有平台接口差异使用宏定义分别实现
      能在2个平台运行发布，让你实现win上开发，linux上发布。      

多协议支持
      网络层能同时支持udp ,tcp协议
      应用层，支持二进制 protobuffer协议，文本支持json协议
      底层支持 websocket协议

框架使用C++结合lua
      网络底层，存储层mysql,redis，sieeson管理，数据收发，处理分包粘包使用C++,给lua层暴露必要接口，屏蔽具体实现。
      应用层使用lua来实现业务逻辑

json/protobuf和lua表的转换
      根据pb的反射机制实现了json/pb和lua表的相互转换，在lua业务层使用lua表非常方便实现协议构造和读取

集成了常用的第三方库,并封装了具体实现
      mysql
      redis
      json
     protobuf
     
#### 联系方式

* qq: 2690540630
