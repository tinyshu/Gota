function print_r ( t )  
    local print_r_cache={}
    local function sub_print_r(t,indent)
        if (print_r_cache[tostring(t)]) then
            print(indent.."*"..tostring(t))
        else
            print_r_cache[tostring(t)]=true
            if (type(t)=="table") then
                for pos,val in pairs(t) do
                    if (type(val)=="table") then
                        print(indent.."["..pos.."] => "..tostring(t).." {")
                        sub_print_r(val,indent..string.rep(" ",string.len(pos)+8))
                        print(indent..string.rep(" ",string.len(pos)+6).."}")
                    elseif (type(val)=="string") then
                        print(indent.."["..pos..'] => "'..val..'"')
                    else
                        print(indent.."["..pos.."] => "..tostring(val))
                    end
                end
            else
                print(indent..tostring(t))
            end
        end
    end
    if (type(t)=="table") then
        print(tostring(t).." {")
        sub_print_r(t,"  ")
        print("}")
    else
        sub_print_r(t,"  ")
    end
    print()
end

--初始化日志模块
Logger_wrapper.init("logger/gateway/", "gateway", true)

local socket_type = {
	TCP_SOCKET = 0,  --tcp
	WEB_SOCKET = 1,  --websocket
}

local proto_type = {
    PROTO_BUF = 0,
	PROTO_JSON = 1,
}

session_wrapper.set_socket_and_proto_type(socket_type.TCP_SOCKET,proto_type.PROTO_BUF)
--protobuf协议，注册cmd
if session_wrapper.get_proto_type() == proto_type.PROTO_BUF then
   local cmd_name_map = require("cmd_name_map")
   if cmd_name_map then
	proto_mgr_wrapper.register_protobuf_cmd(cmd_name_map)
   end
end

--网络服务
netbus_wrapper.tcp_listen("0.0.0.0",6080)
netbus_wrapper.udp_listen("0.0.0.0",8002)

print("start gateway service success tcp:6080 udp:8002!!!!")

--local trm_server = require("trm_server")
local my_service = {
-- msg {1: stype, 2 ctype, 3 utag, 4 body_table_or_str}
on_session_recv_cmd = function(session, msg)
	print_r(msg)
	--table1 = {name="tiny",age=28,id="10086"}
	local t = {stype=1,ctype=1,utag=1001,body={name="shuyi",age=29,email="2654551@qq.com",int_set=10}}
	--local t = {1=1,2=0,3=1001,{name="shuyi",age=29,email="2654551@qq.com",int_set=10}}
	--session_wrapper.close_session(session)
    session_wrapper.send_msg(session,t)
end,

on_session_disconnect = function(session)
end
}

local ret = service_wrapper.register_service(1, my_service)