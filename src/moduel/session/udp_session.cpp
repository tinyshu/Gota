#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../utils/logger.h"
#include "uv.h"
#include "../../moduel/net/net_uv.h"
#include "udp_session.h"
#include "../../moduel/netbus/netbus.h"
#include "../../moduel/netbus/recv_msg.h"
#include "../../proto/proto_manage.h"

udp_recv_buf udp_session::_recv_buf;
void udp_session::close() {

}

static void udp_send_cb(uv_udp_send_t* req, int status) {
	if (status!=0) {
		log_error("udp_send_cb status=%d error=%s", status, uv_strerror(status));
	}

	free(req);
}

void udp_session::send_data(unsigned char* pkg, int pkg_len) {

	uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
	uv_buf_t uv_buf;
	uv_buf.base = (char*)pkg;
	uv_buf.len = pkg_len;
	uv_udp_send(req,this->udp_handle,&uv_buf,1, (const sockaddr*)this->sock_addr, udp_send_cb);
}

void udp_session::send_msg(recv_msg* msg) {
	int pkg_len = 0;
	unsigned char* pkg = protoManager::encode_cmd_msg(msg, &pkg_len);
	if (pkg == NULL || pkg_len == 0) {
		//log
		return;
	}
	send_data(pkg, pkg_len);
	free(pkg);
}

void udp_session::send_raw_msg(raw_cmd* raw_data) {
	send_data(raw_data->raw_data, raw_data->raw_len);
}

static void udp_uv_alloc_buf(uv_handle_t* handle,
	size_t suggested_size,
	uv_buf_t* buf) {

	int need_malloc = suggested_size < 4096 ? 4096 : suggested_size;
	udp_recv_buf* recv_buf = (udp_recv_buf*)handle->data;
	if (recv_buf->max_recv_len < need_malloc) {
		if (recv_buf->recv_buf != NULL) {
			free(recv_buf->recv_buf);
			recv_buf->recv_buf = NULL;
		}

		recv_buf->recv_buf = (unsigned char*)malloc(need_malloc);
		if (recv_buf->recv_buf == NULL) {
			return;
		}
		recv_buf->max_recv_len = need_malloc;
	}
	buf->base = (char*)recv_buf->recv_buf;
	buf->len = recv_buf->max_recv_len;
}


static void after_uv_udp_recv(uv_udp_t* handle,
	ssize_t nread,
	const uv_buf_t* buf,
	const struct sockaddr* addr,
	unsigned flags) {

	/*The receive callback will be called with nread == 0 and addr == NULL when there is nothing to read, 
	and with nread == 0 and addr != NULL when an empty UDP packet is received.*/
	if (nread < 0 ) {
		log_error("after_uv_udp_recv nread=%d\n", nread);
		return;
	}
	if (nread ==0 && addr == NULL) {
		log_error(" nread == 0 and addr == NULL when there is nothing to read!\n");
		return;
	}

	udp_session session;
	session.udp_handle = handle;
	session.sock_addr = (const sockaddr_in*)addr;
	uv_ip4_name((const sockaddr_in*)session.sock_addr, session.address,sizeof(session.address));
	session.port = ntohs(session.sock_addr->sin_port);
	//udp是数据报协议,没有2字节长度的定义
	on_bin_protocal_recv_entry(&session, (unsigned char*)buf->base, nread);
}

void udp_session::start_udp_server(const char* ip,int port) {
	uv_udp_t* udp_server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
	memset(udp_server, 0, sizeof(uv_udp_t));

	uv_udp_init(get_uv_loop(), udp_server);

	struct sockaddr_in addr;
	uv_ip4_addr(ip, port, &addr);
	uv_udp_bind(udp_server, (const struct sockaddr*)&addr, UV_UDP_REUSEADDR);

	memset(&_recv_buf,0,sizeof(udp_recv_buf));
	udp_server->data = (void*)&_recv_buf;
	//在监听recv事件
	uv_udp_recv_start(udp_server, udp_uv_alloc_buf, after_uv_udp_recv);
}
