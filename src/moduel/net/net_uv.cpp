#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "uv.h"
#include "../../utils/logger.h"
#include "net_uv.h"
#include "proto_type.h"
#include "../../moduel/netbus/service_manger.h"

#ifdef WIN32
#include <WinSock2.h>
#include <mswsock.h>
//#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Userenv.lib")

#endif

extern "C" {
#include "../../utils/timer_list.h"
#include "../http_parser/http_parser.h"
#include "../crypt/sha1.h"
#include "../crypt/base64_encoder.h"
#include "../mjson/json.h"
}

#include "../session/tcp_session.h"
#include "../netbus/netbus.h"
#include "../session/session_base.h"
#include "../../utils/mem_manger.h"

#define my_malloc malloc
#define my_free free
#define my_realloc realloc
#define MAX_PKG_SIZE 65536
#define MAX_RECV_SIZE 2048

static char ip_address[64];
static int ip_port;

//netbusģ��ӿ�
extern void init_server_gateway();
extern void exit_server_gateway();
extern void on_bin_protocal_recv_entry(struct session_base* s, unsigned char* data, int len);
extern void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);

//static HANDLE g_iocp = 0;
static void* g_iocp = 0;
 uv_loop_t* loop = NULL;
static uv_connect_t* connect_req;
//����socket����
static uv_tcp_t l_server;

//�洢httpÿ�ν�����ͷ��value
static char header_key[64];
static char client_ws_key[128];
//�Ƿ��������websocket��Sec-WebSocket-Key�ֶ�
static int has_client_key = 0;

struct io_package {
	struct session_base* s;
	int recved; // �յ����ֽ���;
	unsigned char* long_pkg;
	int max_pkg_len;
};

typedef struct {
	uv_write_t req;
	uv_buf_t buf;
} write_req_t;

void init_uv() {
	loop = uv_default_loop();
}

uv_loop_t* get_uv_loop() {
	return loop;
}
//��ܻᴫ��uv_buf_t�øú��������ڴ�
//handle�������¼���uv_tcp_t����
//suggested_size ��ܽ��鱾�η�����ڴ�buff��С
//buf�������ڴ�ָ���ַ
static void on_read_alloc_buff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	struct io_package* io_data = (struct io_package*)handle->data;
	//������Ҫ�Ŀռ�
	int alloc_len = (io_data->recved + suggested_size);
	const int max_buffer_length = MAX_PKG_SIZE - 1;
	alloc_len = (alloc_len > max_buffer_length) ? max_buffer_length : alloc_len;
	if (alloc_len < MAX_RECV_SIZE) {
		//��С�洢�ռ�����2048
		alloc_len = MAX_RECV_SIZE;
	}

	if (alloc_len > io_data->max_pkg_len) {
		io_data->long_pkg = (unsigned char*)my_realloc(io_data->long_pkg, alloc_len + 1);
		io_data->max_pkg_len = alloc_len;
	}

	//���ö�д��ַ�Ϳռ�
	buf->base = (char*)(io_data->long_pkg + io_data->recved);
	buf->len = suggested_size;
}

//���ӹر�
static void on_close_stream(uv_handle_t* peer) {
	struct io_package* io_data = (struct io_package*)peer->data;
	if (io_data->s != NULL) {
		close_session((session*)io_data->s);
		io_data->s = NULL;
	}

	if (io_data->long_pkg!=NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;
	}

	//peer->data
	if (io_data != NULL) {
		my_free(io_data);
		io_data = NULL;
	}

	my_free(peer);
}

static void on_after_shutdown(uv_shutdown_t* req, int status) {
	uv_close((uv_handle_t*)req->handle, on_close_stream);
	my_free(req);
}

static int recv_header(unsigned char* pkg, int len, int* pkg_size) {
	if (len <= 2) { // �յ������ݲ��ܹ������ǵİ��Ĵ�С��������
		return -1;
	}
	//��ȡǰ2���ֽ�,�����ư���ʽ ���ܳ���(2byte)+recv_msg�ṹ+body
	*pkg_size = (pkg[0]) | (pkg[1] << 8);
	return 0;
}

static void on_bin_protocal_recved(struct session* s, struct io_package* io_data) {
	// Step1: �������ݵ�ͷ����ȡ������Ϸ��Э�����Ĵ�С;
	while (io_data->recved > 0) {
		//��ȡ���ݰ�����
		int pkg_size = 0;
		if (recv_header(io_data->long_pkg, io_data->recved, &pkg_size) != 0) { // ����Ͷ��recv����֪���ܷ����һ������ͷ;
			break;
		}
		
		if (pkg_size < 2) {
			//С��2��pack˵��session�쳣��ֱ�ӹر�session
			uv_close((uv_handle_t*)s->c_sock, on_close_stream);
			break;
		}

		// Step2:�ж����ݴ�С���Ƿ񲻷��Ϲ涨�ĸ�ʽ
		if (pkg_size >= MAX_PKG_SIZE) { // ,�쳣�����ݰ���ֱ�ӹرյ�socket;
			uv_close((uv_handle_t*)s->c_sock, on_close_stream);
			break;
		}

		// �����һ�����������ݰ����ͽ�����߼�����
		if (io_data->recved >= pkg_size) { // ��ʾ�����Ѿ��յ����ٳ�����һ���������ݣ�
			unsigned char* pkg_data = io_data->long_pkg;

			//�������pkg_data + 2��ȥ��2�ֽڰ�����
			on_bin_protocal_recv_entry(s, pkg_data + 2, pkg_size - 2);

			//io_data->recved > pkg_size�����������һ�������ݣ������ƶ�ȥ���Ѵ���Ļ������İ�
			if (io_data->recved > pkg_size) {
				memmove(io_data->long_pkg, io_data->long_pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;
			if (io_data->recved<0) {
				io_data->recved = 0;
			}
		}
	}
}

int read_json_tail(unsigned char* pkg_data, int recvlen, int* pkg_size) {
	//����\r\n,ֱ�ӷ��ش���
	if (recvlen < 2) {
		return -1;
	}
#ifdef _DEBUG
	//printf("read_json_tail:%s",pkg_data);
#endif // !_DEBUG

	*pkg_size = 0;
	int i = 0;
	const int len = recvlen - 1;
	while (i < len) {
		if (pkg_data[i] == '\r' && pkg_data[i + 1] == '\n') {
			*pkg_size = (i + 2); //+2��ʾҪ����\r\n
			return 0;
		}
		i++;
	}

	return -1;
}

static void on_json_protocal_recved(struct session* s, struct io_package* io_data) {
	if (s == NULL || io_data == NULL || io_data->long_pkg == NULL) {
		return;
	}
	//io_data->recved��ǰ���������ݴ�С
	while (io_data->recved) {
		//��ǰһ��json����С��һ��������json����\r\n�ָ�
		int pkg_size = 0;
		//��ȡ������ָ��
		unsigned char* pkg_data = io_data->long_pkg;
		if (pkg_data == NULL) {
			log_error("get io_data buffer error\n");
			return;
		}

		//�ָ�һ�������İ�
		if (read_json_tail(pkg_data, io_data->recved, &pkg_size) != 0) {
			//û���ҵ�\r\n,���һ������쳣���ر�session����
			if (io_data->recved > (MAX_PKG_SIZE-1)) {
				uv_close((uv_handle_t*)s->c_sock, on_close_stream);
				break;
			}
		}

		//�ߵ������ʾ������һ�����������ݰ�
		//�����ϲ㴦����
		on_json_protocal_recv_entry(s, pkg_data, pkg_size);
		//�������������������ǰ��pkg_size , 
		//���io_data->recved == pkg_size��ʾ����û�����ݣ�������memmove��
		if (io_data->recved > pkg_size) {
			memmove(pkg_data, pkg_data + pkg_size, io_data->recved - pkg_size);
		}
		io_data->recved -= pkg_size;
		//������û������
		if (io_data->recved <=0 && io_data->long_pkg!=NULL) {
			my_free(io_data->long_pkg);
			io_data->long_pkg = NULL;
			io_data->max_pkg_len = 0;
			break;
		}
	}

}

static int on_header_field(http_parser* p, const char *at,size_t length) {
	size_t len = (length < 63) ? length : 64;
	strncpy(header_key, at, len);
	header_key[len] = 0;

	return 0;
}

static int on_header_value(http_parser* p, const char *at,size_t length) {
	if (strcmp(header_key, "Sec-WebSocket-Key") != 0) {
		return 0;
	}
	strncpy(client_ws_key, at, length);
	client_ws_key[length] = 0;
	has_client_key = 1;
	return 0;
}

//���ݷ��ͳɹ���ص�
static void after_write(uv_write_t* req, int status) {
	write_req_t* wr =(write_req_t*)req;
	memory_mgr::get_instance().free_memory(wr->buf.base);
	memory_mgr::get_instance().free_memory(wr);
	
	if (status == 0){
		return;
	}

	log_error("uv_write error: %s - %s\n", uv_err_name(status), uv_strerror(status));
}

void uv_send_data(void* stream, char* pkg, unsigned int pkg_len) {
	if (stream == NULL || pkg == NULL || pkg_len <= 0) {
		return;
	}

	write_req_t* wr = (write_req_t*)memory_mgr::get_instance().alloc_memory(sizeof(write_req_t));
	if (wr == NULL) {
		log_error("malloc faild in uv_send_data\n");
		return;
	}

	unsigned char* send_buf = (unsigned char*)memory_mgr::get_instance().alloc_memory(pkg_len + 1);
	if (send_buf == NULL) {
		//my_free(wr);
		memory_mgr::get_instance().free_memory(wr);
		log_error("malloc send_buf faild in uv_send_data\n");
		return;
	}
	memcpy(send_buf,pkg, pkg_len);
	wr->buf = uv_buf_init((char*)send_buf, pkg_len);
	if (uv_write(&wr->req, (uv_stream_t*)stream, &wr->buf, 1, after_write)) {
		memory_mgr::get_instance().free_memory(send_buf);
		memory_mgr::get_instance().free_memory(wr);
		log_error("uv_write failed");
	}
}
//��ȡһ��������֡
//ws_sizeһ������ws������
//head_len������
static int process_websocket_pack(unsigned char* pkg, int pkg_len, int* head_len, int* ws_size) {
	unsigned char* mask = NULL;
	unsigned char* rawdata = NULL;
	int datalen = 0;
	unsigned char chlen = pkg[1];
	chlen = chlen & 0x7f; //ȥ�����λ��1 0x7f = 0111ffff
	if (chlen <= 125) {
		//chlen�������ݳ���
		if (pkg_len < 2 + 4) {
			return -1;
		}
		datalen = chlen;
		mask = (unsigned char*)&(pkg[2]);
	}
	else if (chlen == 126) {
		//7+16
		datalen = pkg[2] + (pkg[3] << 8); //�����data[3]�൱�ڶ����Ƶ�ʮλ
		if (pkg_len < 4 + 4) {
			return -1;
		}
		mask = (unsigned char*)&(pkg[4]);
	}
	else if (chlen == 127) {
		//����8���ֽڱ�ʾ����һ���޿������ϣ�����ǰ��32λ�Ϳ�����'7+64
		datalen = pkg[2] + (pkg[3] << 8) + (pkg[4] << 16) + (pkg[5] << 24);
		if (pkg_len < 2 + 8 + 4) {
			return -1;
		}
		mask = (unsigned char*)&(pkg[6]);
	}
	//������ʼ��ַ
	rawdata = (unsigned char*)(mask + 4);
	*head_len = (int)(rawdata - pkg);
	*ws_size = *head_len + datalen;
	return 0;
}

static int parser_websocket_pack(struct session* s, unsigned char* body, int len, unsigned char* mask, int protocal_type) {
	if (s == NULL || body == NULL || mask == NULL) {
		printf("parser_websocket_pack parament error\n");
		return -1;
	}
	//ʹ��mask����body,���벻��ı����ݳ���
	for (int i = 0; i < len; ++i) {
		int j = i % 4;
		body[i] = body[i] ^ mask[j];
	}

#if _DEBUG
	char buf[1024];
	memcpy_s(buf, sizeof(buf), body, len);
	buf[len] = 0;
	printf("websocket data:  %s", buf);
#endif

	if (BIN_PROTOCAL == protocal_type) {
		on_bin_protocal_recv_entry(s, body, len);
	}
	else if (JSON_PROTOCAL == protocal_type) {
		on_json_protocal_recv_entry(s, body, len);
	}

	return 0;
}

static int process_websocket_data(struct session* s, struct io_package* io_data, int protocal_type) {
	if (s == NULL || io_data == NULL) {
		printf("process_websocket_data parament error\n");
		return -1;
	}

	unsigned char* pkg = io_data->long_pkg;
	while (io_data->recved > 0) {
		int pkg_size = 0;
		int header_size = 0;
		//if (0x81 == pkg[0] || 0x82 == pkg[0]) {
		//��ȡһ��������֡�����ز�����0˵��û�ж�ȡ��һ��������֡ 
		if (process_websocket_pack(pkg, io_data->recved, &header_size, &pkg_size) != 0) {
			break;
		}

		//������һ��������֡���ж�֡�Ϸ���
		if (pkg_size >= MAX_PKG_SIZE) {
			uv_close((uv_handle_t*)s->c_sock, on_close_stream);
			break;
		}
		//��������������һ��������֡
		if (pkg_size <= io_data->recved) {
#if _DEBUG
			printf("websocket pack: header_size:%d body_size:%d\n", header_size, pkg_size);
#endif
			//�����һ���ֽ���0x88 websocket�ͻ�������ر�
			if (0x88 == pkg[0]) {
				uv_close((uv_handle_t*)s->c_sock, on_close_stream);
				break;
			}
			//�������ݰ�
			parser_websocket_pack(s, pkg + header_size, pkg_size - header_size, pkg + header_size - 4, protocal_type);

			if (io_data->recved > pkg_size) {
				memmove(pkg, pkg + pkg_size, io_data->recved - pkg_size);
			}
			//������ݿ�����С�������洢����copy��С������
			io_data->recved -= pkg_size;

			if (io_data->recved == 0 && io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
				io_data->max_pkg_len = 0;
				break;
			}
		}
	}

	return 0;
}

//����websocket����Э��
static int process_websocket_connect(struct session* s, struct io_package* io_data, char* ip, int port) {
	if (s == NULL || io_data == NULL) {
		printf("process_websocket_connect parament error\n");
		return -1;
	}

	struct http_parser p;
	http_parser_init(&p, HTTP_REQUEST);

	struct http_parser_settings setting;
	http_parser_settings_init(&setting);

	//on_header_fieldÿ������һ��httpͷ��field�ֶα�����
	//on_header_valueÿ�ν�����ͷ���ֶα�����
	setting.on_header_field = on_header_field;
	setting.on_header_value = on_header_value;
	unsigned char* pkg = io_data->long_pkg;

	/*
	������߼���:
	�ж�has_client_key==0˵����û�ж�ȡ��ͷ����Sec-WebSocket-Key�ֶΣ���Ҫ��
	��Ͷ�ݶ����󣬵�on_header_value��ȡ��Sec-WebSocket-Key�ֶκ�has_client_key==1
	˵��Sec-WebSocket-Key�Ѿ��洢��client_ws_key
	*/
	//s->has_client_key = 0;
	//���Զ����session,��on_header_value�ص�has_client_key����Ϊ1
	has_client_key = 0;
	http_parser_execute(&p, &setting, (const char*)pkg, io_data->recved);
	if (0 == has_client_key) {
		s->is_shake_hand = 0;
		//websocket�����ֽ׶Σ�����յ���package����MAX_RECV_SIZE��ʾ������
		if (io_data->recved >= MAX_RECV_SIZE) {
			uv_close((uv_handle_t*)s->c_sock, on_close_stream);
			return -1;
		}

		return -1;
	}

	int sha1_len = 0;
	int base64_len = 0;
	static char bufferreq[256] = { 0 };
	const char* migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	sprintf(bufferreq, "%s%s", client_ws_key, migic);

	char* sha1_str = crypt_sha1((uint8_t*)bufferreq, strlen(bufferreq), &sha1_len);
	char* bs_str = base64_encode((uint8_t*)sha1_str, sha1_len, &base64_len);

	char buffer[1024];
	strncpy(buffer, bs_str, base64_len);
	buffer[base64_len] = 0;

	const char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n" \
		"Upgrade:websocket\r\n" \
		"Connection: Upgrade\r\n" \
		"Sec-WebSocket-Accept: %s\r\n" \
		"WebSocket-Location: ws://%s:%d/chat\r\n" \
		"WebSocket-Protocol:chat\r\n\r\n";

	static char accept_buffer[256];
	sprintf(accept_buffer, wb_accept, buffer, ip, port);
	
	s->is_shake_hand = 1;
	uv_send_data(s->c_sock, accept_buffer, (int)strlen(accept_buffer));
	if (bs_str!=NULL) {
		base64_encode_free(bs_str,strlen(bs_str));
	}
	
	if (io_data->long_pkg != NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;

	}
	io_data->recved = 0;
	io_data->max_pkg_len = 0;

	return 0;
}

//libuv��ȡ�����ݻص�����
static void on_after_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
	if (nread < 0) {
		uv_shutdown_t* sreq = NULL;
		sreq = (uv_shutdown_t*)my_malloc(sizeof(uv_shutdown_t));
		uv_shutdown(sreq, stream, on_after_shutdown);
		return;
	}

	if (nread == 0) {
		/* Everything OK, but nothing read. */
		return;
	}

	struct io_package* io_data = (struct io_package*)stream->data;
	if (io_data) {
		io_data->recved += nread;
	}

	int protocal_type = get_proto_type();

	struct session* s = (struct session*)io_data->s;
	if (s->socket_type == TCP_SOCKET_IO) {
		if (protocal_type == BIN_PROTOCAL) {
			//tcp+������Э��
			//������(2�ֽ�) + ��ͷ(8�ֽ�) + body(protobuf)
			on_bin_protocal_recved(s, io_data);
		}
		else if (protocal_type == JSON_PROTOCAL) {
			//tcp+json�ı�Э��
			on_json_protocal_recved(s, io_data);
		}
	}
	else if (s->socket_type == WEB_SOCKET_IO) {
		//websocketЭ�飬�����wsЭ�鲻��Ҫ����nginx
		//�����wssЭ�飬��Ҫ��ʹ��nginxת��
		if (s->is_shake_hand == 0) {
			//����websokcetЭ�鴦��
			process_websocket_connect(s, io_data, ip_address, ip_port);
		}
		else {
			//����websocket����
			process_websocket_data(s, io_data, protocal_type);
		}
	}

}

//���������ӳɹ������ӽ�����,��������ص�����
static void on_connection(uv_stream_t* server, int status) {
	if (status<0) {
		return;
	}
	uv_tcp_t* new_client = (uv_tcp_t*)my_malloc(sizeof(uv_tcp_t));
	if (NULL == new_client) {
		return;
	}
	memset(new_client, 0, sizeof(uv_tcp_t));
	//��������ӵ�eventloop
	uv_tcp_init(loop, new_client);
	int ret = uv_accept(server, (uv_stream_t*)new_client);
	if (ret < 0){
		my_free(new_client);
		new_client = NULL;
		return;
	}
	//��������Ҫ��2������ �����Զ��������� ��Ӽ������¼�
	struct io_package* io_data;
	io_data = (struct io_package*)my_malloc(sizeof(struct io_package));
	if(io_data==NULL){
		my_free(new_client);
		new_client = NULL;
		return;
	}
	//���Լ��Ľ��ܻ���ṹ
	new_client->data = io_data;
	memset(new_client->data, 0, sizeof(struct io_package));
	io_data->max_pkg_len = MAX_RECV_SIZE;
	struct session* s = save_session(new_client, "127.0.0.1", 100);
	
	/*
	uv_tcp_t new_client��libuv���tcp���Ӷ���
	session s �ǿ�ܱ���ͻ��˻ػ�����,�ǿ�ܱ�ʾһ���ͻ�������,�ϲ����ݷ��Ͷ��ǵ��øö���ӿ�
	io_package io_data ��ǰ���ܵ�������һ��������
	*/
	s->socket_type = (int)(server->data);
	io_data->s = s;
	io_data->long_pkg = NULL;
	//�������Ӱ󶨹��ĵ��¼��ͻص�
	//on_read_alloc_buff ���ж��¼��������ú����ᱻ�ص�
	//on_read_alloc_buff �����ݽ��������ڷ������ݶ�ȡ�������ڸú���ʵ��
	//on_after_read ��ȡ�����ݺ󱻵���
	uv_read_start((uv_stream_t*)new_client, on_read_alloc_buff, on_after_read);

	//֪ͨlua���������ӳɹ��¼�
	server_manage::get_instance().on_session_connect(s);

}

void start_server_ws(char* ip, int port) {
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));

	uv_tcp_init(uv_default_loop(), listen);

	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);

	int ret = uv_tcp_bind(listen, (const struct sockaddr*) &addr, 0);
	if (ret != 0) {
		// printf("bind error\n");
		free(listen);
		return;
	}

	uv_listen((uv_stream_t*)listen, SOMAXCONN, on_connection);
	listen->data = (void*)WEB_SOCKET_IO;
}

void start_server(char* ip, int port) {
	//����һ���¼�ѭ������
	if (loop==NULL) {
		loop = uv_default_loop();
	}
	
	uv_tcp_init(loop, &l_server);
	struct sockaddr_in addr;
	strncpy(ip_address,ip,strlen(ip));
	ip_port = port;
	uv_ip4_addr(ip, port, &addr);
	//��tcp�ϼ�����ip��port
	int ret = uv_tcp_bind(&l_server, (const struct sockaddr*)&addr, 0);
	if (ret != 0) {
		goto failed;
	}
	//����һ�������׽��֣����������ӣ��ص�on_connection����
	ret = uv_listen((uv_stream_t*)&l_server, SOMAXCONN, on_connection);
	if (ret != 0) {
		goto failed;
	}
	l_server.data = (void*)TCP_SOCKET_IO;
	//�����¼�ѭ��
	//uv_run(loop, UV_RUN_DEFAULT);
failed:

	return;
}

void run() {
	uv_run(loop, UV_RUN_DEFAULT);
}

typedef struct connect_context {
	void(*on_connected)(const char* err, session_base* s, void* udata);
	void* udata;
}connect_context;

//���ӳɹ��ص�����
static void on_after_connect(uv_connect_t* handle, int status) {
	connect_context* context = (connect_context*)handle->data;
	if (status) {
		const char* uv_error = uv_strerror(status);
		log_error("connect error");
		if (context->on_connected != NULL) {
			context->on_connected(uv_error, NULL, context->udata);
		}

		uv_close((uv_handle_t*)handle->handle, on_close_stream);
		free(handle->data); //connect_context
		free(handle);
		return;
	}
	
	
	if (context->on_connected!=NULL) {
		io_package* package = (io_package*)handle->handle->data;
		context->on_connected(NULL, (session_base*)package->s, context->udata);
	}

	int iret = uv_read_start(handle->handle, on_read_alloc_buff, on_after_read);
	if (iret) {
		log_error("uv_read_start error");
		return;
	}

	
	free(handle->data); //connect_context
	free(handle);
}

struct session* netbus_connect(char* server_ip, int port) {

	struct sockaddr_in bind_addr;
	int iret = uv_ip4_addr(server_ip, port, &bind_addr);
	if (iret) {
		return NULL;
	}

	uv_tcp_t* stream = (uv_tcp_t*)my_malloc(sizeof(uv_tcp_t));
	uv_tcp_init(loop, stream);

	struct io_package* io_data;
	io_data = (struct io_package*)my_malloc(sizeof(struct io_package));
	memset(io_data, 0, sizeof(struct io_package));
	stream->data = io_data;

	
	struct session* s = save_session(stream, server_ip, port);
	io_data->max_pkg_len = 0;
	io_data->s = s;
	io_data->long_pkg = NULL;
	s->socket_type = TCP_SOCKET_IO;
	s->is_server_session = 1;

	connect_req = (uv_connect_t*)my_malloc(sizeof(uv_connect_t));
	
	iret = uv_tcp_connect(connect_req, stream, (struct sockaddr*)&bind_addr, on_after_connect);
	if (iret) {
		log_error("uv_tcp_connect error!!!");
		return NULL;
	}

	return s;
}

void tcp_connect(const char* server_ip, int port,
				void(*connect_cb)(const char* err, session_base* s, void* udata), 
				void* udata) {
	struct sockaddr_in bind_addr;
	int ret = uv_ip4_addr(server_ip, port, &bind_addr);
	if (ret) {
		return;
	}
	struct io_package* io_data;
	io_data = (struct io_package*)my_malloc(sizeof(struct io_package));
	memset(io_data, 0, sizeof(struct io_package));
	
	connect_req = (uv_connect_t*)my_malloc(sizeof(uv_connect_t));
	uv_tcp_t* stream = (uv_tcp_t*)my_malloc(sizeof(uv_tcp_t));
	stream->data = io_data;
	uv_tcp_init(loop, stream);

	struct session* s = save_session(stream, const_cast<char*>(server_ip), port);
	io_data->max_pkg_len = 0;
	io_data->s = s;
	io_data->long_pkg = NULL;
	s->socket_type = TCP_SOCKET_IO;
	s->is_server_session = 1;

	connect_context* context = (connect_context*)my_malloc(sizeof(connect_context));
	context->on_connected = connect_cb;
	context->udata = udata;
	connect_req->data = context;

	uv_tcp_connect(connect_req, stream, (struct sockaddr*)&bind_addr, on_after_connect);
}