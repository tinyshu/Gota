local stype = require("service_type")
local Cmd = require("cmd_type")
local guest = require("auth_server/guest")

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

--定义认证服务器协议和函数映射
local auth_service_handles = {}
auth_service_handles[Cmd.GuestLoginReq] = guest.login
-----------------------------------------------

-- {stype, ctype, utag, body}
function on_auth_recv_cmd(s, msg)
	--解析数据做响应的逻辑
	--print(msg[1],msg[2],msg[3],msg[4])
	print_r(msg[4])
	--判断cmdid是否有对应的处理函数
	local ctype = msg[2] --协议id
	if auth_service_handles[ctype] then
	   auth_service_handles[ctype](s,msg)
	end


	--print_r(msg)
	--直接返回msg[3]就是网关生成的utag
	--pb格式数据返回
	--local ret_msg = {stype=stype.AuthSerser,ctype=2,utag=msg[3],body={status=200}}
	--json回数据包 msg[4]直接返回请求的json,正常逻辑需要自己需要响应的json的数据包
	--local ret_msg = {stype=stype.AuthSerser, ctype=Cmd.eLoginRes , utag=msg[3], body=msg[4]}
	--session_wrapper.send_msg(s,ret_msg)
end

function on_auth_session_disconnect(s,ctype) 
end

local auth_service = {
	on_session_recv_cmd = on_auth_recv_cmd,
	on_session_disconnect = on_auth_session_disconnect,
}

return auth_service