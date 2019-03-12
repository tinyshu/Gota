--lua测试文件，vs里调试需要放在bin目录下
--连接mysql redis需要填自己对应的mysql redis连接信息
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

function PrintTable(table , level)
  print("call PrintTable")
  level = level or 1
  local indent = ""
  for i = 1, level do
    indent = indent.."  "
  end

  if key ~= "" then
    print(indent..key.." ".."=".." ".."{")
  else
    print(indent .. "{")
  end

  key = ""
  for k,v in pairs(table) do
     if type(v) == "table" then
        key = k
        PrintTable(v, level + 1)
     else
        local content = string.format("%s%s = %s", indent .. "  ",tostring(k), tostring(v))
      print(content)  
      end
  end
  print(indent .. "}")

end

--connect(const char* ip, int port, const char* db_name, const char* user_name,const char* passwd, cb_connect_db connect_db)

mysql_wrapper.connect("ip",port,"user_center","root","123321",function(err, context) 
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

--读写redis
--redis_wrapper.connect("ip",port,5,function(err, context)
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
---我是测试test1


