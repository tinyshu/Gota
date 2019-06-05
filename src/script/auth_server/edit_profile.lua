--锟轿匡拷锟斤拷锟较编辑模锟斤拷锟竭硷�?
local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local mysql_center = require("database/mysql_auth_center")
local utils = require("utils")


---- {stype, ctype, utag, body}
function edit_profile(s,msg)
	local uid = msg[3]
	print("edit_profile uid:"..uid)
	
	if uid <=0 then
	   print("edit_profile uid is error!")
	   return 
	end

	local edit_profile_req = msg[4]
	--锟斤拷证锟斤拷锟斤拷锟斤拷锟�?
	if string.len(edit_profile_req.unick) <0  or 
	 (edit_profile_req.uface<0 and edit_profile_req.uface>9) or 
	 (edit_profile_req.usex~=0 and edit_profile_req.usex~=1) then
	      print("edit_profile InvaildErr")
	      local ret_msg = {
					       stype=stype_module.AuthSerser,ctype=cmd_module.EditProfileRes,utag=uid,
							body={
									status = res_module.InvaildErr
							}}

			session_wrapper.send_msg(s,ret_msg)
			return 
			
	 end 

	 --锟斤拷锟斤拷center锟斤拷锟捷匡�?db锟侥诧拷锟斤拷锟斤拷锟斤拷mysql_auth_center.lua模锟斤拷
	 local ret_handle = function(err,res)
	 print("edit_profile_info call back!")
		local ret = res_module.OK
	    if err ~= nil then
		    ret = res_module.SystemErr
		end
		local ret_msg = {
			stype=stype_module.AuthSerser,ctype=cmd_module.EditProfileRes,utag=uid,
			 body={
					 status = ret,
			 }}

		session_wrapper.send_msg(s,ret_msg)
         
     end
     print("mysql_center.edit_profile_info call!")
	 mysql_center.edit_profile_info(uid,edit_profile_req.unick,edit_profile_req.uface,edit_profile_req.usex,ret_handle)
end

--锟斤拷锟斤拷模锟介函锟斤�?
local editProfile = {
	edit = edit_profile
}

return editProfile
