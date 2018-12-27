#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uv.h"
#include "../../moduel/net/net_uv.h"
#include "udp_session.h"
#include "../../moduel/netbus/netbus.h"
#include "../../moduel/netbus/recv_msg.h"
udp_recv_buf udp_session::_recv_buf;
udp_session g_udp_session;

void udp_session::close() {

}

void udp_session::send_data(unsigned char* pkg, int pkg_len) {

}

void udp_session::send_msg(recv_msg* msg) {

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
		buf->base = (char*)recv_buf->recv_buf;
		buf->len = recv_buf->max_recv_len;
	}
}


static void after_uv_udp_recv(uv_udp_t* handle,
	ssize_t nread,
	const uv_buf_t* buf,
	const struct sockaddr* addr,
	unsigned flags) {

	/*udp_session udp_s;
	udp_s.udp_handler = handle;
	udp_s.addr = addr;
	uv_ip4_name((struct sockaddr_in*)addr, udp_s.c_address, 32);
	udp_s.c_port = ntohs(((struct sockaddr_in*)addr)->sin_port);*/
	on_bin_protocal_recv_entry(&g_udp_session, (unsigned char*)buf->base, nread);
}

void udp_session::start_udp_server() {
	uv_udp_t* udp_server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
	memset(udp_server, 0, sizeof(uv_udp_t));

	uv_udp_init(get_uv_loop(), udp_server);

	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", 8802, &addr);
	uv_udp_bind(udp_server, (const struct sockaddr*)&addr, 0);

	memset(&_recv_buf,0,sizeof(udp_recv_buf));
	udp_server->data = (void*)&_recv_buf;
	//ÔÚ¼àÌýrecvÊÂ¼þ
	uv_udp_recv_start(udp_server, udp_uv_alloc_buf, after_uv_udp_recv);
}
