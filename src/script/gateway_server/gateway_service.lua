-- raw_data由C++测那个推送的完整数据包
function on_gw_recv_raw_cmd(s, raw_data)
end

function on_gw_session_disconnect(s) 
end

local gw_service = {
	on_session_recv_raw_cmd = on_gw_recv_raw_cmd,
	on_session_disconnect = on_gw_session_disconnect,
}

return gw_service