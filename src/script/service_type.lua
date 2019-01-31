--定义各个服务的service_type,service_typezai gateway做转发到其他服务需要根据service_type来路由
local serivce_type = {
  AuthSerser=1,
  SystemServer=2,
  LogicServer=3,
}

return serivce_type