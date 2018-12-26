--LOGDEBUG("hello")
--myname = "tinyshu"
--a=2
--b=5
--local sum = Add(a+b)
--print(sum)

--arr={"a001","a002","a003"}
--print_array(arr)

--table1 = {name="tiny",age=28,id="10086"}

--print_table(table1)

--local info_table = re_table()
--print(info_table["name"]..info_table["age"]) 

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

--connect(const char* ip, int port, const char* db_name, const char* user_name,const char* passwd, cb_connect_db connect_db)
--[[
mysql_wrapper.connect("123.206.46.126",3306,"user_center","root","123321",function(err, context) 
--	log_debug("event call");

	if(err)  then
		print(err)
		return
	end

    mysql_wrapper.query(context, "select * from t_user_info", function (err, ret)
		if err then 
			print(err)
			return;
		end

		print("success")
		--print(ret)
		print_r(ret)
	end)
	
	mysql_wrapper.close(context);
end)
]]

--读写redis
--redis_wrapper.connect("123.206.46.126",6379,5,function(err, context)
	--print("redis call");

--	if(err) then
--		print(err)
--		return
--	end
	
--	redis_wrapper.query(context, "get name1", function (err, result)
--		if(err) then
--			print(err)
--			return
--		end
--		print(result)
		
--		redis_wrapper.query(context, "set name tiny", function (err, result)
--		    redis_wrapper.close(context)
--		end)
		
--		print("return")
		
--	end)
--end);

local my_service = {
-- msg {1: stype, 2 ctype, 3 utag, 4 body_table_or_str}
on_session_recv_cmd = function(session, msg)
	print_r(msg)
	--table1 = {name="tiny",age=28,id="10086"}
	local t = {stype=1,ctype=0,utag=1001,body={name="shuyi",age=29,email="2654551@qq.com",int_set=10}}
	--local t = {1=1,2=0,3=1001,{name="shuyi",age=29,email="2654551@qq.com",int_set=10}}
	--session_wrapper.close_session(session)
    session_wrapper.send_msg(session,t)
end,

on_session_disconnect = function(session)
end
}

local ret = service_wrapper.register_service(1, my_service)