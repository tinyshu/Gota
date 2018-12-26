#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "export_tcpsession.h"
#include "../../moduel/netbus/recv_msg.h"
#include "tcp_session.h"
#include "../../proto/proto_manage.h"
void export_tcp_session::close() {
	close_session((session*)_tcp_session);
}

void export_tcp_session::send_data(unsigned char* pkg, int pkg_len) {
	session_send((session*)_tcp_session, pkg, pkg_len);
}

void export_tcp_session::send_msg(recv_msg* msg) {
	//先把message编码成unsigned char在发送
	int pkg_len = 0;
	unsigned char* pkg = proroManager::encode_cmd_msg(msg,&pkg_len);
	if (pkg==NULL || pkg_len==0) {
		//log
		return;
	}
	session_send((session*)_tcp_session, pkg, pkg_len);
}

session_base* export_tcp_session::get_inner_session() {
	return _tcp_session;
}