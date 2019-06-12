local stype_module = require("service_type")
local cmd_module = require("cmd_type")
local res_module = require("respones")
local utils = require("utils")
local player = require("logic_server/logic_player")
local zone =   require("logic_server/Zone")
local room_moduel = require("logic_server/room")
local room_status = require("logic_server/room_status")

--uid��player�Ķ�Ӧ��ϵ
local online_player_map = {}
local online_player_num = 0

--ƥ���б�zone_wait_list
-- zone_wait_list[Zone.SGYD] = {} --> uid --> p;
--ÿ����ͼid��Ϊkey, value�������һ����match
--match = {uid,player}
local zone_wait_list = {} 

--ƥ��ɹ����������б�,Ҳ�ǰ���zid������
--room_list[zid]��ȡȫ��zid��ͼ�ķ����б�list
local room_list = {}
room_list[zone.SGYD] = {}
room_list[zone.ASSY] = {}



--�ҵ�һ��״̬��InView�ķ���
local function search_inview_match_mgr(zid)
    if zid < 0 then
	   return nil
	end
	
	--��ȡzid��ͼȫ���ż�list
	zid_room_list = room_list[zid]
	if zid_room_list == nil then
	    print("search_inview_match_mgr zid_room_list is nil")
	    return nil
	end
    
	local k,v
	for k,v in pairs(zid_room_list) do
	   if v.room_state == room_status.InView then
	      return v
	   end

	   --�жϵ�ǰ�����Ƿ�����

	end
	--�Ҳ���,����һ����room���󷵻�
	print("room:new()")
	local croom = room:new()
	croom:init(zid)
	table.insert(zid_room_list,croom)
	return croom
end

--ƥ�䶨ʱ���������жϵȴ��б�zone_wait_listƥ���߼�
--һ����ʱ�����ú�������һ����ͼ��ƥ��
function do_match_sgyd_map()
 
	--�ȴ�zone.SGYD��ͼ��ȫ������б�
	if #zone_wait_list == 0  then
	   --print("zone_wait_list len:"..#zone_wait_list)
	   return
	end
	print("zone_wait_list len:"..#zone_wait_list)
	local zid, wait_list
	--�������еĵȴ��б�
	for zid, wait_list in pairs(zone_wait_list) do 
	    print("zid, wait_list in pairs(zone_wait_list)")
	    local k,v
	    for k,v in pairs(wait_list) do
	        --k:uid v:player
	        local room =  search_inview_match_mgr(zid)
	        if room ~= nil then
			   print("find room:enter_room uid:"..k)
	           if room:enter_room(v) == false then
			      --����
				  print("room:enter_room error! v.status:"..v.status)
			   else
			      --����ɹ����ӵȴ��б��Ƴ�
				  wait_list[k] = nil
			   end

	        end
	    end
	end

end


--1sһ��ƥ��,ע����һ����Ҫ�ŵ�������������
init_count = 0
if init_count==0 then
   timer_wrapper.create_timer(do_match_sgyd_map,-1,1000,1000)
   init_count = init_count + 1
end


local function send_logic_enter_status(s,uid,sstype,sstatus)	
 
  local ret_msg = {
				   stype=sstype,ctype=cmd_module.LoginLogicRes,utag=uid,
				   body={
							status = sstatus
					    }}
                    
					--utils.print_table(ret_msg)
			        session_wrapper.send_msg(s,ret_msg)
end

--������Ϸ�߼�������
function login_server_enter(s,msg)
  
   local uid = msg[3]
   --[[
   	 logic��Ϊ��ս��ͼ��������������������
	 1.һ��logic������һ�������Ķ�ս������������֧��N����ͬ�ĵ�ͼ��N�ֲ�ͬ���淨
	 2.�Ѳ�ͬ�ĵ�ͼ���ֵ���ͬ��logic_server�ϣ�Ȼ��ʹ��ÿ��logic��stype��ͬ������Ϊ���صķ�ʽ
   ]]
   --�ͻ��˽����stype
   local stype = msg[1] 
   print("login_server_enter uid:"..uid.."stype"..stype)
   
   local play = online_player_map[uid]
   if play ~= nil then
      --����session,�����session�ǿͻ��˺�gateway�����Ӷ���
      play:set_session(s)
	  send_logic_enter_status(s,uid,stype,res_module.OK)
	  return
   end

   --û���ҵ�������һ��playerʵ��
   play = player:new()
   if play == nil then
      print("player:new() is error")
	  return
   end

   --��db��ȡplayer����
   play:init(uid, s, function(status)
      if status == res_module.OK then
	        if online_player_map[uid] == nil then
			   online_player_map[uid] = play
			   online_player_num = online_player_num + 1
			   print("...online_player_num:"..online_player_num)
			end
		end
		send_logic_enter_status(s,uid,stype,status)
   end)
end

--���ع㲥�������û�������Ϣ
function on_player_disconnect(s,msg)
    local uid = msg[3]
	print("on_player_disconnect uid:"..uid)
	
	play = online_player_map[uid]
	if play == nil then
	   return
	end

	--�ж�����Ƿ��ڵȴ��б�
	if play.zid ~= -1 then
	   --�Ƴ��ȴ��б�
	   zone_wait_list[play.zid][uid] = nil
	   play.zid = -1
	end
	--�Ȳ����Ƕ���������ֱ������online_player_mapɾ��
	if online_player_map[uid] ~= nil then
	   print("on_player_disconnect online_player_map remove uid:"..uid)
	   online_player_map[uid] = nil
	   online_player_num = online_player_num -1
	   if online_player_num < 0 then
	      online_player_num = 0
	   end  
	   print("on_player_disconnect online_player_num:"..online_player_num)
	end

end

--��gateway���ط������
--�����ضϿ��󣬻������ز෢����������
function on_gateway_disconnect(s,ctype)
	local k, v
	--����ֻɾ��player������洢��session,online_player_map�����
	--2��ԭ��:
	--1.����ͻ��˲�û�к����ضϿ���Ҳ����˵client��gateway�������Ǻõģ�
	--2.�ں�gateway�Ͽ�������Ҫ����ȥ�������صģ�������ӳɹ����ڰ�client��gateway
	--���Ӻõ�session�������õ�player����Ϳ�����
    for k, v in pairs(online_player_map) do 
		v:set_session(nil)
	end

end

--gateway���ӳɹ����������������֪ͨ��������service
function on_gateway_connect(s,stype)
	--print("on_gateway_connect stype"..stype)
	 for k, v in pairs(online_player_map) do 
		v:set_session(s)
	end

end

function logic_enter_zone(s,msg)
    local stype =  msg[1]
	local uid = msg[3]
	local zid = msg[4].zid
	print("logic_enter_zone stype:"..stype.." uid:"..uid.." zid:"..zid)
	if uid <= 0 then
	     local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvaildErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		 return
	end

	--�жϽ���ĵ�ͼ�Ƿ�Ϸ�
	if zid ~= zone.SGYD and zid ~= zone.ASSY then
		 local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvaildErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		return
	end

	play = online_player_map[uid]
	--�Ҳ��������Ѿ��ڵ�ͼ�У��޷��ڽ���
	if play == nil or play.zid ~= -1 then
	    print("play.zid:"..play.zid)
	    local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.InvalidOptErr
					    }}
                    
		 session_wrapper.send_msg(s,ret_msg)
		 return
	end

	--��һ������һ���յ��б�
	if not zone_wait_list[zid] then
	   zone_wait_list[zid] = {}
	end
	--����play����ƥ���б�
	play.zid = zid
	zone_wait_list[zid][uid] = play
	print("zone_wait_list[zid][uid] zid:"..zid.." uid:"..uid)
	local ret_msg = {
			       stype=stype,ctype=cmd_module.EnterZoneRes,utag=uid,
				   body={
							status = res_module.ok
					    }}
                    
    session_wrapper.send_msg(s,ret_msg)

end

local game_mgr = {
	login_server_enter = login_server_enter,
	on_player_disconnect = on_player_disconnect,
	on_gateway_disconnect = on_gateway_disconnect,
	on_gateway_connect = on_gateway_connect,
	logic_enter_zone = logic_enter_zone,
}

return game_mgr