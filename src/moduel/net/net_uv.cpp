#include<stdio.h>
#include<string.h>
#include <stdlib.h>

#include "net_uv.h"


#ifdef WIN32
#include <WinSock2.h>
#include <mswsock.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "IPHLPAPI.lib")
#pragma comment(lib, "Psapi.lib")
#pragma comment(lib, "Userenv.lib")

#endif

extern "C" {
#include "../../utils/log.h"
#include "../../utils/timer_list.h"
#include "../../3rd/http_parser/http_parser.h"
#include "../../3rd/crypt/sha1.h"
#include "../../3rd/crypt/base64_encoder.h"
#include "../../3rd/mjson/json.h"
}
#include "../session/tcp_session.h"
#include "../netbus/netbus.h"


#define my_malloc malloc
#define my_free free
#define my_realloc realloc
#define MAX_PKG_SIZE 65536
#define MAX_RECV_SIZE 2048

static char ip_address[64];
static int ip_port;

//netbus模块接口
extern void init_server_gateway();
extern void exit_server_gateway();
extern void on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len);
extern void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);

static HANDLE g_iocp = 0;
 uv_loop_t* loop = NULL;
static uv_connect_t* connect_req;
//监听socket对象
static uv_tcp_t l_server;

//存储http每次解析的头部value
static char header_key[64];
static char client_ws_key[128];
//是否解析到了websocket的Sec-WebSocket-Key字段
static int has_client_key = 0;

struct io_package {
	struct session* s;
	int recved; // 收到的字节数;
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
//框架会传入uv_buf_t让该函数分配内存
//handle触发读事件的uv_tcp_t对象
//suggested_size 框架建议本次分配的内存buff大小
//buf创建的内存指针地址
static void on_read_alloc_buff(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
	struct io_package* io_data = (struct io_package*)handle->data;
	int alloc_len = (io_data->recved + suggested_size);
	const int max_buffer_length = MAX_PKG_SIZE - 1;
	alloc_len = (alloc_len > max_buffer_length) ? max_buffer_length : alloc_len;
	if (alloc_len < MAX_RECV_SIZE) {
		//最小存储空间申请2048
		alloc_len = MAX_RECV_SIZE;
	}

	if (alloc_len > io_data->max_pkg_len) {
		io_data->long_pkg = (unsigned char*)my_realloc(io_data->long_pkg, alloc_len + 1);
		io_data->max_pkg_len = alloc_len;
	}

	//设置读写地址和空间
	buf->base = (char*)(io_data->long_pkg + io_data->recved);
	buf->len = suggested_size;
}

//连接关闭
static void on_close_stream(uv_handle_t* peer) {
	struct io_package* io_data = (struct io_package*)peer->data;
	if (io_data->s != NULL) {
		close_session(io_data->s);
		io_data->s = NULL;
	}

	if (io_data->long_pkg!=NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;
	}

	if (peer->data != NULL) {
		my_free(peer->data);
		peer->data = NULL;
	}

	my_free(peer);
}

static void on_after_shutdown(uv_shutdown_t* req, int status) {
	uv_close((uv_handle_t*)req->handle, on_close_stream);
	my_free(req);
}

static int recv_header(unsigned char* pkg, int len, int* pkg_size) {
	if (len <= 2) { // 收到的数据不能够将我们的包的大小解析出来
		return -1;
	}
	//读取前2个字节,二进制包格式 包总长度(2byte)+recv_msg结构+body
	*pkg_size = (pkg[0]) | (pkg[1] << 8);
	return 0;
}

static void on_bin_protocal_recved(struct session* s, struct io_package* io_data) {
	// Step1: 解析数据的头，获取我们游戏的协议包体的大小;
	while (io_data->recved > 0) {
		int pkg_size = 0;
		if (recv_header(io_data->long_pkg, io_data->recved, &pkg_size) != 0) { // 继续投递recv请求，知道能否接收一个数据头;
			break;
		}

		// Step2:判断数据大小，是否不符合规定的格式
		if (pkg_size >= MAX_PKG_SIZE) { // ,异常的数据包，直接关闭掉socket;
			uv_close((uv_handle_t*)s->c_sock, on_close_stream);
			break;
		}

		// 是否收完了一个数据包;
		if (io_data->recved >= pkg_size) { // 表示我们已经收到至少超过了一个包的数据；
			unsigned char* pkg_data = io_data->long_pkg;

			//printf("%s", pkg_data + 4);
			on_bin_protocal_recv_entry(s, pkg_data + 2, pkg_size - 2);

			if (io_data->recved > pkg_size) { // 1.5 个包
				memmove(io_data->long_pkg, io_data->long_pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->recved ==0 && io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}
		}
	}
}

int read_json_tail(unsigned char* pkg_data, int recvlen, int* pkg_size) {
	//不足\r\n,直接返回错误
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
			*pkg_size = (i + 2); //+2表示要算上\r\n
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
	//io_data->recved当前缓存区数据大小
	while (io_data->recved) {
		//当前一个json包大小，一个完整的json包用\r\n分割
		int pkg_size = 0;
		//获取缓存区指针
		unsigned char* pkg_data = io_data->long_pkg;
		if (pkg_data == NULL) {
			LOGERROR("get io_data buffer error\n");
			return;
		}

		//分割一个完整的包
		if (read_json_tail(pkg_data, io_data->recved, &pkg_size) != 0) {
			//没有找到\r\n,并且缓存区异常，关闭session连接
			if (io_data->recved > (MAX_PKG_SIZE-1)) {
				uv_close((uv_handle_t*)s->c_sock, on_close_stream);
				break;
			}
		}

		//走到这里表示解析到一个完整的数据包
		//调用上层处理函数
		on_json_protocal_recv_entry(s, pkg_data, pkg_size);
		//处理完这个包，缓存区前移pkg_size , 
		//如果io_data->recved == pkg_size表示缓存没有数据，不用在memmove了
		if (io_data->recved > pkg_size) {
			memmove(pkg_data, pkg_data + pkg_size, io_data->recved - pkg_size);
		}
		io_data->recved -= pkg_size;
		//缓存区没有数据
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

//数据发送成功后回调
static void after_write(uv_write_t* req, int status) {
	write_req_t* wr =(write_req_t*)req;
	my_free(wr->buf.base);
	my_free(wr);

	if (status == 0){
		return;
	}

	LOGERROR("uv_write error: %s - %s\n", uv_err_name(status), uv_strerror(status));
}

void uv_send_data(void* stream, char* pkg, unsigned int pkg_len) {
	if (stream == NULL || pkg == NULL || pkg_len <= 0) {
		return;
	}

	write_req_t* wr = (write_req_t*)my_malloc(sizeof(write_req_t));
	if (wr == NULL) {
		LOGERROR("malloc faild in uv_send_data\n");
		return;
	}

	unsigned char* send_buf = (unsigned char*)my_malloc(pkg_len + 1);
	if (send_buf == NULL) {
		my_free(wr);
		LOGERROR("malloc send_buf faild in uv_send_data\n");
		return;
	}
	memcpy(send_buf,pkg, pkg_len);
	wr->buf = uv_buf_init((char*)send_buf, pkg_len);
	if (uv_write(&wr->req, (uv_stream_t*)stream, &wr->buf, 1, after_write)) {
		my_free(send_buf);
		my_free(wr);
		LOGERROR("uv_write failed");
	}
}
//读取一个完整的帧
//ws_size一个完整ws包长度
//head_len包长度
static int process_websocket_pack(unsigned char* pkg, int pkg_len, int* head_len, int* ws_size) {
	unsigned char* mask = NULL;
	unsigned char* rawdata = NULL;
	int datalen = 0;
	unsigned char chlen = pkg[1];
	chlen = chlen & 0x7f; //去掉最高位的1 0x7f = 0111ffff
	if (chlen <= 125) {
		//chlen就是数据长度
		if (pkg_len < 2 + 4) {
			return -1;
		}
		datalen = chlen;
		mask = (unsigned char*)&(pkg[2]);
	}
	else if (chlen == 126) {
		//7+16
		datalen = pkg[2] + (pkg[3] << 8); //这里的data[3]相当于二进制的十位
		if (pkg_len < 4 + 4) {
			return -1;
		}
		mask = (unsigned char*)&(pkg[4]);
	}
	else if (chlen == 127) {
		//这里8个字节表示长度一般无可用用上，解析前面32位就可以了'7+64
		datalen = pkg[2] + (pkg[3] << 8) + (pkg[4] << 16) + (pkg[5] << 24);
		if (pkg_len < 2 + 8 + 4) {
			return -1;
		}
		mask = (unsigned char*)&(pkg[6]);
	}
	//数据起始地址
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
	//使用mask解码body,解码不会改变数据长度
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
		//读取一个完整的帧，返回不等于0说明没有读取到一个完整的帧 
		if (process_websocket_pack(pkg, io_data->recved, &header_size, &pkg_size) != 0) {
			break;
		}

		//解析到一个完整的帧，判断帧合法性
		if (pkg_size >= MAX_PKG_SIZE) {
			uv_close((uv_handle_t*)s->c_sock, on_close_stream);
			break;
		}
		//缓存区里至少有一个完整的帧
		if (pkg_size <= io_data->recved) {
#if _DEBUG
			printf("websocket pack: header_size:%d body_size:%d\n", header_size, pkg_size);
#endif
			//如果第一个字节是0x88 websocket客户端请求关闭
			if (0x88 == pkg[0]) {
				uv_close((uv_handle_t*)s->c_sock, on_close_stream);
				break;
			}
			//处理数据包
			parser_websocket_pack(s, pkg + header_size, pkg_size - header_size, pkg + header_size - 4, protocal_type);

			if (io_data->recved > pkg_size) {
				memmove(pkg, pkg + pkg_size, io_data->recved - pkg_size);
			}
			//如果数据可以在小缓冲区存储，就copy到小缓冲区
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

//处理websocket握手协议
static int process_websocket_connect(struct session* s, struct io_package* io_data, char* ip, int port) {
	if (s == NULL || io_data == NULL) {
		printf("process_websocket_connect parament error\n");
		return -1;
	}

	struct http_parser p;
	http_parser_init(&p, HTTP_REQUEST);

	struct http_parser_settings setting;
	http_parser_settings_init(&setting);

	//on_header_field每解析到一个http头部field字段被调用
	//on_header_value每次解析到头部字段被调用
	setting.on_header_field = on_header_field;
	setting.on_header_value = on_header_value;
	unsigned char* pkg = io_data->long_pkg;

	/*
	这里的逻辑是:
	判断has_client_key==0说明还没有读取到头部的Sec-WebSocket-Key字段，需要在
	次投递读请求，到on_header_value读取到Sec-WebSocket-Key字段后，has_client_key==1
	说明Sec-WebSocket-Key已经存储到client_ws_key
	*/
	//s->has_client_key = 0;
	//绑定自定义的session,在on_header_value回调has_client_key设置为1
	has_client_key = 0;
	http_parser_execute(&p, &setting, (const char*)pkg, io_data->recved);
	if (0 == has_client_key) {
		s->is_shake_hand = 0;
		//websocket在握手阶段，如果收到的package大于MAX_RECV_SIZE表示出错了
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

//接受到数据
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

	struct session* s = io_data->s;
	if (s->socket_type == TCP_SOCKET_IO) {
		if (protocal_type == BIN_PROTOCAL) {
			//tcp+二进制协议
			on_bin_protocal_recved(s, io_data);
		}
		else if (protocal_type == JSON_PROTOCAL) {
			//tcp+json文本协议
			on_json_protocal_recved(s, io_data);
		}
	}
	else if (s->socket_type == WEB_SOCKET_IO) {
		if (s->is_shake_hand == 0) {
			process_websocket_connect(s, io_data, ip_address, ip_port);
		}
		else {
			process_websocket_data(s, io_data, protocal_type);
		}
	}

}

//新连接回调函数
static void on_connection(uv_stream_t* server, int status) {
	uv_tcp_t* new_client = (uv_tcp_t*)my_malloc(sizeof(uv_tcp_t));
	if (NULL == new_client) {
		return;
	}
	memset(new_client, 0, sizeof(uv_tcp_t));
	//添加新连接到eventloop
	uv_tcp_init(loop, new_client);
	int ret = uv_accept(server, (uv_stream_t*)new_client);

	struct io_package* io_data;
	io_data = (struct io_package*)my_malloc(sizeof(struct io_package));
	new_client->data = io_data;
	io_data->max_pkg_len = MAX_RECV_SIZE;
	memset(new_client->data, 0, sizeof(struct io_package));

	struct session* s = save_session(new_client, "127.0.0.1", 100);
	s->socket_type = get_socket_type();
	io_data->s = s;
	io_data->long_pkg = NULL;
	//给新连接绑定关心的事件和回调
	uv_read_start((uv_stream_t*)new_client, on_read_alloc_buff, on_after_read);
	
	
}

void start_server(char* ip, int port, int socket_type, int protocal_type) {
	//创建一个事件循环对象
	if (loop==NULL) {
		loop = uv_default_loop();
	}
	
	uv_tcp_init(loop, &l_server);
	struct sockaddr_in addr;
	//uv_ip4_addr(ip, port, &addr);
	strncpy(ip_address,ip,strlen(ip));
	ip_port = port;
	uv_ip4_addr("0.0.0.0", port, &addr);
	int ret = uv_tcp_bind(&l_server, (const struct sockaddr*)&addr, 0);
	if (ret != 0) {
		goto failed;
	}

	ret = uv_listen((uv_stream_t*)&l_server, SOMAXCONN, on_connection);
	if (ret != 0) {
		goto failed;
	}

	//进入事件循环
	uv_run(loop, UV_RUN_DEFAULT);
failed:
	
	return;
}

static void on_after_connect(uv_connect_t* handle, int status) {
	if (status) {
		LOGERROR("connect error");
		uv_close((uv_handle_t*)handle->handle, on_close_stream);
		return;
	}

	int iret = uv_read_start(handle->handle, on_read_alloc_buff, on_after_read);
	if (iret) {
		LOGERROR("uv_read_start error");
		return;
	}
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
		LOGERROR("uv_tcp_connect error!!!");
		return NULL;
	}

	return s;
}