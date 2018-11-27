#include<stdio.h>
#include<string.h>
#include <stdlib.h>
#include "net_io.h"
#include <WinSock2.h>
#include <mswsock.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "odbc32.lib")
#pragma comment(lib, "odbccp32.lib")
#pragma comment(lib,"Mswsock.lib") 


#include "../session/tcp_session.h"
#include "../../3rd/http_parser/http_parser.h"
#include "../../3rd/crypt/sha1.h"
#include "../../3rd/crypt/base64_encoder.h"
#include "../../3rd/mjson/json.h"

#include "../services/service_gateway.h"
#include "../../utils/log.h"
//#include "../service/types_service.h"
//#include "../service/table_service.h"

//#include "../server.h"
#define my_malloc malloc
#define my_free free
#define my_realloc realloc

char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n" \
"Upgrade:websocket\r\n" \
"Connection: Upgrade\r\n" \
"Sec-WebSocket-Accept: %s\r\n" \
"WebSocket-Location: ws://%s:%d/chat\r\n" \
"WebSocket-Protocol:chat\r\n\r\n";

extern void init_server_gateway();
extern void exit_server_gateway();
extern void on_bin_protocal_recv_entry(struct session* s, unsigned char* data, int len);
extern void on_json_protocal_recv_entry(struct session* s, unsigned char* data, int len);
static HANDLE g_iocp = 0;
enum {
	IOCP_ACCPET = 0,
	IOCP_RECV,
	IOCP_WRITE,
};

//存储http每次解析的头部value
static char header_key[64];
//是否解析到了websocket的Sec-WebSocket-Key字段
static int has_client_key = 0; 

#define MAX_RECV_SIZE 2047
#define MAX_PKG_SIZE ((1<<16) - 1)
struct io_package {
	WSAOVERLAPPED overlapped;
	int opt; // 标记一下我们当前的请求的类型;
	int accpet_sock; //关联的socket
	WSABUF wsabuffer;
	int recved; // 收到的字节数;
	unsigned char* long_pkg;  //大缓存区，当小缓冲区不够，使用这个指针
	int max_pkg_len; //当前缓存区大小
	unsigned char pkg[MAX_RECV_SIZE]; //小缓存区，
};

static void
post_accept(SOCKET l_sock, HANDLE iocp) {
	struct io_package* pkg = malloc(sizeof(struct io_package));
	memset(pkg, 0, sizeof(struct io_package));

	pkg->wsabuffer.buf = pkg->pkg;
	pkg->wsabuffer.len = MAX_RECV_SIZE - 1;
	pkg->opt = IOCP_ACCPET;
	pkg->max_pkg_len = MAX_RECV_SIZE - 1;

	DWORD dwBytes = 0;
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	int addr_size = (sizeof(struct sockaddr_in) + 16);
	pkg->accpet_sock = client;

	AcceptEx(l_sock, client, pkg->wsabuffer.buf, 0/*pkg->wsabuffer.len - addr_size* 2*/,
		addr_size, addr_size, &dwBytes, &pkg->overlapped);
}

static void post_recv(SOCKET client_fd, HANDLE iocp) {
	// 异步发送请求;
	// 什么是异步? recv 8K数据，架设这个时候，没有数据，
	// 普通的同步(阻塞)线程挂起，等待数据的到来;
	// 异步就是如果没有数据发生，也会返回继续执行;
	struct io_package* io_data = malloc(sizeof(struct io_package));
	// 清0的主要目的是为了能让overlapped清0;
	memset(io_data, 0, sizeof(struct io_package));

	io_data->opt = IOCP_RECV;
	io_data->wsabuffer.buf = io_data->pkg;
	io_data->wsabuffer.len = MAX_RECV_SIZE - 1;
	io_data->max_pkg_len = MAX_RECV_SIZE - 1;
	// 发送了recv的请求;
	// 
	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	int ret = WSARecv(client_fd, &(io_data->wsabuffer),
		1, &dwRecv, &dwFlags,
		&(io_data->overlapped), NULL);
	if (0 != ret) {
		;
		//printf("WSARecv error:%d", GetLastError());
	}
}

// 解析到头的回掉函数
static char header_key[64];
static char client_ws_key[128];

static int on_header_field(http_parser* p, const char *at,
	size_t length) {
	size_t len = (length < 63) ? length : 64;
	strncpy(header_key, at, len);
	header_key[len] = 0;
	
	return 0;
}

static int on_header_value(http_parser* p, const char *at,
	size_t length) {
	if (strcmp(header_key, "Sec-WebSocket-Key") != 0) {
		return 0;
	}
	strncpy(client_ws_key, at, length);
	client_ws_key[length] = 0;
	has_client_key = 1;
	printf("%s\n", client_ws_key);
	return 0;
}

static int recv_header(unsigned char* pkg, int len, int* pkg_size) {
	if (len <= 1) { // 收到的数据不能够将我们的包的大小解析出来
		return -1;
	}

	*pkg_size = (pkg[0]) | (pkg[1] << 8);
	return 0;
}

static void on_bin_protocal_recved(struct session* s, struct io_package* io_data) {
	// Step1: 解析数据的头，获取我们游戏的协议包体的大小;
	while (io_data->recved > 0) {
		int pkg_size = 0;
		if (recv_header(io_data->pkg, io_data->recved, &pkg_size) != 0) { // 继续投递recv请求，知道能否接收一个数据头;
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;

			io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
			io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

			int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}

		// Step2:判断数据大小，是否不符合规定的格式
		if (pkg_size >= MAX_PKG_SIZE) { // ,异常的数据包，直接关闭掉socket;
			close_session(s);
			my_free(io_data); // 释放这个socket使用的完成端口的io_data;
			break;
		}

		// 是否收完了一个数据包;
		if (io_data->recved >= pkg_size) { // 表示我们已经收到至少超过了一个包的数据；
			unsigned char* pkg_data = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;

			printf("%s", pkg_data + 4);
			on_bin_protocal_recv_entry(s, pkg_data + 2, pkg_size - 2);

			if (io_data->recved > pkg_size) { // 1.5 个包
				memmove(io_data->pkg, io_data->pkg + pkg_size, io_data->recved - pkg_size);
			}
			io_data->recved -= pkg_size;

			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}

			if (io_data->recved == 0) { // 重新投递请求
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
				io_data->wsabuffer.len = MAX_RECV_SIZE - io_data->recved;

				int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}
		}
		else { // 没有收完一个数据包，所以我们直接投递recv请求;
			unsigned char* recv_buffer = io_data->pkg;
			if (pkg_size > MAX_RECV_SIZE) {
				if (io_data->long_pkg == NULL) {
					io_data->long_pkg = my_malloc(pkg_size + 1);
					memcpy(io_data->long_pkg, io_data->pkg, io_data->recved);
				}
				recv_buffer = io_data->long_pkg;
			}

			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			io_data->wsabuffer.buf = recv_buffer + io_data->recved;
			io_data->wsabuffer.len = pkg_size - io_data->recved;

			int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
		}
		// end 
	}
}

int read_json_tail(unsigned char* pkg_data, int recvlen, int* pkg_size) {
	//不足\r\n,直接返回错误
	if (recvlen < 2) {
		return -1;
	}
#ifdef _DEBUG
	
	printf("read_json_tail:%s",pkg_data);
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
	if (s == NULL || io_data == NULL) {
		return;
	}
	//io_data->recved当前缓存区数据大小
	while (io_data->recved) {
		//当前一个json包大小，一个完整的json包用\r\n分割
		int pkg_size = 0;
		//获取缓存区指针
		unsigned char* pkg_data = io_data->long_pkg == NULL ? io_data->pkg : io_data->long_pkg;
		if (pkg_data == NULL) {
			printf("get io_data buffer error\n");
			return;
		}

		//分割一个完整的包
		if (read_json_tail(pkg_data, io_data->recved, &pkg_size) != 0) {
			//没有找到\r\n,并且缓存区异常，关闭session连接
			if (io_data->recved > MAX_PKG_SIZE) {
				close_session(s);
				my_free(io_data);
				break;
			}

			//判断当前空间是否需要扩容
			if (io_data->recved >= io_data->max_pkg_len) {
				//按照当前已有数据2倍申请
				int new_alloc_len = io_data->recved * 2;
				new_alloc_len = (new_alloc_len > MAX_PKG_SIZE) ? MAX_PKG_SIZE : new_alloc_len;
				if (io_data->long_pkg == NULL) {
					io_data->long_pkg = my_malloc(new_alloc_len + 1);
					memcpy_s(io_data->long_pkg, new_alloc_len, io_data->pkg, io_data->recved);
				}
				else {
					io_data->long_pkg = my_realloc(io_data->long_pkg, new_alloc_len + 1);
				}

				io_data->max_pkg_len = new_alloc_len;
			}
			//在投递一个请求
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;
			unsigned char* buf = (io_data->long_pkg != NULL) ? io_data->long_pkg : io_data->pkg;
			io_data->wsabuffer.buf = buf + io_data->recved;
			io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
			int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
				1, &dwRecv, &dwFlags,
				&(io_data->overlapped), NULL);
			break;
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
		if (0 >= io_data->recved) {
			io_data->recved = 0;
			//先做内存回收，在使用io_data里的小缓存空间
			if (io_data->long_pkg != NULL) {
				my_free(io_data->long_pkg);
				io_data->long_pkg = NULL;
			}
			//在投递一个读请求
			DWORD dwRecv = 0;
			DWORD dwFlags = 0;

			io_data->max_pkg_len = MAX_RECV_SIZE;
			io_data->wsabuffer.buf = io_data->pkg;
			io_data->wsabuffer.len = io_data->max_pkg_len;

			int r = WSARecv(s->c_sock, &(io_data->wsabuffer), 1, &dwRecv, &dwFlags, &(io_data->overlapped), NULL);
			break;
		}
	}
}

//读取一个完整的帧
//ws_size一个完整ws包长度
//head_len包长度
static int process_websocket_pack(unsigned char* pkg,int pkg_len,int* head_len,int* ws_size) {
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

static int parser_websocket_pack(struct session* s,unsigned char* body,int len, unsigned char* mask, int protocal_type) {
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
	memcpy_s(buf,sizeof(buf), body, len);
	buf[len] = 0;
	printf("websocket data:  %s",buf);
#endif

	if (BIN_PROTOCAL == protocal_type) {
		on_bin_protocal_recv_entry(s, body, len);
	}
	else if (JSON_PROTOCAL == protocal_type) {
		on_json_protocal_recv_entry(s, body, len);
	}

	return 0;
}

//处理websocket协议 
static int process_websocket_data(struct session* s, struct io_package* io_data,int protocal_type) {
	if (s == NULL || io_data == NULL) {
		printf("process_websocket_data parament error\n");
		return -1; 
	}

	//循环解析包，集中情况
	//解析到一个websocket完整的帧，传递给上层应用，然后继续投递读请求
	//没有解析到完整的websocket帧包，继续投递读请求
	//没有解析到完整的websocket帧包,并且缓存区满，说明包格式有错误，断开连接
	unsigned char* pkg = io_data->long_pkg == NULL ? io_data->pkg : io_data->long_pkg;
	while (io_data->recved >0) {
		int pkg_size = 0;
		int header_size = 0;
		//if (0x81 == pkg[0] || 0x82 == pkg[0]) {
			//读取一个完整的帧，返回不等于0说明没有读取到一个完整的帧 
			if (process_websocket_pack(pkg, io_data->recved,&header_size,&pkg_size) != 0) {
				//继续投递
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = pkg + io_data->recved;
				io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
				int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}

			//解析到一个完整的帧，判断帧合法性
			if (pkg_size >= MAX_PKG_SIZE) {
				close_session(s);
				if (io_data->long_pkg) {
					my_free(io_data->long_pkg);
				}
				
				my_free(io_data);
				io_data = NULL;
				break;
			}
			//缓存区里至少有一个完整的帧
			if (pkg_size <= io_data->recved) {
#if _DEBUG
				printf("websocket pack: header_size:%d body_size:%d\n", header_size, pkg_size);
#endif
				//如果是0x88 websocket客户端请求关闭
				if (0x88 == pkg[1]) {
					if (NULL != io_data->long_pkg) {
						my_free(io_data->long_pkg);
						my_free(io_data);
						io_data = NULL;
					}
					close_session(s);
					break;
				}
				//处理数据包
				parser_websocket_pack(s, pkg + header_size, pkg_size- header_size, pkg+ header_size-4, protocal_type);
				
				if (io_data->recved > pkg_size) {
					memmove(pkg, pkg + pkg_size, io_data->recved - pkg_size);
				}
				//如果数据可以在小缓冲区存储，就copy到小缓冲区
				io_data->recved -= pkg_size;
				if (io_data->recved < MAX_RECV_SIZE && io_data->long_pkg != NULL) {
					memcpy(io_data->pkg, io_data->long_pkg, io_data->recved);
					my_free(io_data->long_pkg);
					io_data->long_pkg = NULL;
				}

				if (0 == io_data->recved) {
					//缓存区没有数据在投递读
					DWORD dwRecv = 0;
					DWORD dwFlags = 0;
					io_data->wsabuffer.buf = io_data->pkg;
					io_data->wsabuffer.len = MAX_RECV_SIZE;
					int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
						1, &dwRecv, &dwFlags,
						&(io_data->overlapped), NULL);
					break;
				}
			}
			else {
				//这里说明一个帧头部有了，但还没有完整的帧，需要继续收数据
				if (pkg_size > MAX_RECV_SIZE) {
					//大于小缓冲区，需要重新申请空间
					if (io_data->long_pkg == NULL) {
						io_data->long_pkg = my_malloc(pkg_size + 1);
						memcpy_s(io_data->long_pkg, pkg_size + 1, pkg, io_data->recved);
					}
					pkg = io_data->long_pkg;
					io_data->max_pkg_len = pkg_size + 1;
				} //if

				//在投递读请求
				DWORD dwRecv = 0;
				DWORD dwFlags = 0;
				io_data->wsabuffer.buf = pkg + io_data->recved;
				io_data->wsabuffer.len = pkg_size - io_data->recved;
				int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
					1, &dwRecv, &dwFlags,
					&(io_data->overlapped), NULL);
				break;
			}
		//}
	}

	return 0;
}

//处理websocket握手协议
static int process_websocket_connect(struct session* s, struct io_package* io_data, char* ip,int port) {
	if (s==NULL || io_data==NULL) {
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
	unsigned char* pkg = io_data->long_pkg == NULL ? io_data->pkg : io_data->long_pkg;
	
	/*
	这里的逻辑是:
	判断has_client_key==0说明还没有读取到头部的Sec-WebSocket-Key字段，需要在
	次投递读请求，到on_header_value读取到Sec-WebSocket-Key字段后，has_client_key==1
	说明Sec-WebSocket-Key已经存储到client_ws_key
	*/
	has_client_key = 0;
	http_parser_execute(&p, &setting, pkg, io_data->recved);
	if (0 == has_client_key) {
		s->is_shake_hand = 0;
		//不正常的握手包;
		if (io_data->recved >= MAX_RECV_SIZE) { 
			close_session(s);
			my_free(io_data);
			return -1;
		}

		//在次投递一个读请求
		DWORD dwRecv = 0;
		DWORD dwFlags = 0;

		if (io_data->long_pkg != NULL) {
			my_free(io_data->long_pkg);
			io_data->long_pkg = NULL;
		}

		io_data->max_pkg_len = MAX_RECV_SIZE;
		io_data->wsabuffer.buf = &(io_data->pkg[io_data->recved]);
		io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

		int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),1, &dwRecv, &dwFlags, &(io_data->overlapped),NULL);
		return -1;
	}

	int sha1_len = 0;
	int base64_len = 0;
	static char bufferreq[256] = {0};
	const char* migic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	sprintf(bufferreq, "%s%s", client_ws_key, migic);

	char* sha1_str = crypt_sha1((uint8_t*)bufferreq, strlen(bufferreq), &sha1_len);
	char* bs_str = base64_encode((uint8_t*)sha1_str, sha1_len, &base64_len);

	char buffer[1024] = {0};
	strncpy(buffer, bs_str, base64_len);
	buffer[base64_len] = '\0';

	char *wb_accept = "HTTP/1.1 101 Switching Protocols\r\n" \
		"Upgrade:websocket\r\n" \
		"Connection: Upgrade\r\n" \
		"Sec-WebSocket-Accept: %s\r\n" \
		"WebSocket-Location: ws://%s:%d/chat\r\n" \
		"WebSocket-Protocol:chat\r\n\r\n";

	static char accept_buffer[256];
	sprintf(accept_buffer, wb_accept, buffer, ip, port);

	send(s->c_sock, accept_buffer, strlen(accept_buffer), 0);
	s->is_shake_hand = 1;

	//连接成功在投递一个请求
	DWORD dwRecv = 0;
	DWORD dwFlags = 0;
	if (io_data->long_pkg != NULL) {
		my_free(io_data->long_pkg);
		io_data->long_pkg = NULL;

	}
	//连接成功后数据清0
	io_data->recved = 0;
	io_data->max_pkg_len = MAX_RECV_SIZE;
	io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
	io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;

	int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
		1, &dwRecv, &dwFlags,
		&(io_data->overlapped), NULL);

	return 0;
}

struct session* gateway_connect(char* server_ip, int port,int stype) {
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET) {
		return NULL;
	}
	struct sockaddr_in sockaddr;
	sockaddr.sin_addr.S_un.S_addr = inet_addr(server_ip);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_port = htons(port);

	int ret = connect(sock, ((struct sockaddr*) &sockaddr), sizeof(sockaddr));
	if (ret != 0) {
		closesocket(sock);
		return NULL;
	}

	struct session* s = save_session(sock, inet_ntoa(sockaddr.sin_addr), ntohs(sockaddr.sin_port));
	//连接后端服务使用tcp+json协议
	s->socket_type = TCP_SOCKET_IO;
	CreateIoCompletionPort((HANDLE)sock, g_iocp, (DWORD)s, 0);
	post_recv((SOCKET)sock, g_iocp);

	return s;
}

void start_server(char* ip, int port, int socket_type, int protocal_type) {

	WSADATA data;
	WSAStartup(MAKEWORD(2, 2), &data);
	// 新建一个完成端口;
	SOCKET l_sock = INVALID_SOCKET;
	HANDLE iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (iocp == NULL) {
		goto failed;
	}
	g_iocp = iocp;
	// 创建一个线程
	// CreateThread(NULL, 0, ServerWorkThread, (LPVOID)iocp, 0, 0);
	// end

	// 创建客户端监听socket
	l_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (l_sock == INVALID_SOCKET) {
		goto failed;
	}
	// bind socket
	struct sockaddr_in s_address;
	memset(&s_address, 0, sizeof(s_address));
	s_address.sin_family = AF_INET;
	s_address.sin_addr.s_addr = inet_addr(ip);
	s_address.sin_port = htons(port);

	if (bind(l_sock, (struct sockaddr *) &s_address, sizeof(s_address)) != 0) {
		goto failed;
	}

	if (listen(l_sock, SOMAXCONN) != 0) {
		goto failed;
	}

	// start 
	CreateIoCompletionPort((HANDLE)l_sock, iocp, (DWORD)0, 0);
	post_accept(l_sock, iocp);
	
	// end 

	DWORD dwTrans;
	struct session* s;
	//  当我们有完成事件发生了以后,
	// GetQueuedCompletionStatus 会返回我们请求的
	// 时候的WSAOVERLAPPED 的地址,根据这个地址，找到
	// io_data, 找到了io_data,也就意味着我们找到了,
	// 读的缓冲区；
	struct io_package* io_data;
	
	const char* socket_type_str = conver_socket_type_str(socket_type);
	const char* protocal_str = conver_protocal_str(protocal_type);
	printf("server start [ip:%s, port:%d, socket_type:%s, protocal_type:%s]\n",
		ip, port, socket_type_str, protocal_str);
	while (1) {
		clear_offline_session();
		int m_sec = -1;
		if (NULL != GATEWAY_TIMER_LIST) {
			m_sec = update_timer_list(GATEWAY_TIMER_LIST);
		}
		
		// 阻塞函数，当IOCP唤醒这个线程来处理已经发生事件
		// 的时候，才会把这个线程唤醒;
		// IOCP 与select不一样，等待的是一个完成的事件;
		s = NULL;
		dwTrans = 0;
		io_data = NULL;
		//GetQueuedCompletioStatus参数
		//iocp完成端口
		//dwTrans本次实际读取到的字节数 
		//s在CreateIoCompletionPort调用关联的第三个参数
		//io_data这个是在WSARecv设置的WSAOVERLAPPED参数
		//WSAOVERLAPPED一个放在自定义结构体第一个字段，这样就可以和自定义结构体关联
		int timeout_sec = m_sec > 0 ? m_sec : WSA_INFINITE;
		int ret = GetQueuedCompletionStatus(iocp, &dwTrans, (LPDWORD)&s, (LPOVERLAPPED*)&io_data, timeout_sec);
		if (ret == 0) {
			
			//if (WAIT_TIMEOUT == GetLastError()) {
			//	//printf("iocp time out\n");
			//	continue;
			//}
			if (dwTrans==0 && io_data!=NULL) {
				//说明关联到完成端口的socket关闭,对方主动关闭或者异常崩溃
				if (io_data->long_pkg != NULL) {
					my_free(io_data->long_pkg);
					io_data->long_pkg = NULL;
				}
				//如果是server session要调用lost_server_connect
				
				close_session(s); 
				my_free(io_data);
			}
			else if (io_data!=NULL) {
				//如果ret==0并且io_data不为NULL，说明不是超时，而是IOCP发生了错误
				LOGWARNING("error iocp %d\n", GetLastError());
				if (io_data->opt == IOCP_ACCPET) {
					closesocket(io_data->accpet_sock);
					post_accept(l_sock, iocp, io_data);
				}
				else if (io_data->opt == IOCP_RECV) {
					DWORD dwRecv = 0;
					DWORD dwFlags = 0;
					if (io_data->long_pkg != NULL) {
						my_free(io_data->long_pkg);
						io_data->long_pkg = NULL;

					}
					io_data->max_pkg_len = MAX_RECV_SIZE;
					io_data->wsabuffer.buf = io_data->pkg + io_data->recved;
					io_data->wsabuffer.len = io_data->max_pkg_len - io_data->recved;
					int ret = WSARecv(s->c_sock, &(io_data->wsabuffer),
						1, &dwRecv, &dwFlags,
						&(io_data->overlapped), NULL);
				}
			}
			continue;
		}
		// IOCP端口唤醒了一个工作线程，
		// 来告诉用户有socket的完成事件发生了;
		// printf("IOCP have event\n");
		if (dwTrans == 0 && io_data->opt == IOCP_RECV) { // socket 关闭
			close_session(s);
			free(io_data);
			continue;
		}// end

		switch (io_data->opt) {
		case IOCP_RECV: {
			io_data->recved += dwTrans;
			if (s->socket_type == TCP_SOCKET_IO) {
				if (protocal_type == BIN_PROTOCAL) {
					on_bin_protocal_recved(s, io_data);
				}
				else if (protocal_type == JSON_PROTOCAL) {
					on_json_protocal_recved(s, io_data);
				}
			}
			else if (s->socket_type == WEB_SOCKET_IO) {
				//先判断是否握手成功
				if (0 == s->is_shake_hand) {
					//处理websocket握手
					process_websocket_connect(s, io_data,ip,port);
				}
				else {
					//处理websocket数据协议
					//判断第一个字节是否为websocket帧格式
					process_websocket_data(s, io_data, protocal_type);
				
				}
		
			}
		}break;
		case IOCP_ACCPET:
		{
			int client_fd = io_data->accpet_sock;
			int addr_size = (sizeof(struct sockaddr_in) + 16);
			struct sockaddr_in* l_addr = NULL;
			int l_len = sizeof(struct sockaddr_in);

			struct sockaddr_in* r_addr = NULL;
			int r_len = sizeof(struct sockaddr_in);

			GetAcceptExSockaddrs(io_data->wsabuffer.buf,
				0, /*io_data->wsabuffer.len - addr_size * 2, */
				addr_size, addr_size,
				(struct sockaddr**)&l_addr, &l_len,
				(struct sockaddr**)&r_addr, &r_len);
			//创建一个session添加到列表，把session关联到新socket
			struct session* s = save_session(client_fd, inet_ntoa(r_addr->sin_addr), ntohs(r_addr->sin_port));
			//设置成启动服务设置的socket_type
			s->socket_type = socket_type;
			//关联完成端口句柄和新连接句柄，并设置lpCompletionKey
			//这个lpCompletionKey参数在GetQueuedCompletioStatus时候返回
			CreateIoCompletionPort((HANDLE)client_fd, iocp, (DWORD)s, 0);
			post_recv(client_fd, iocp);
			post_accept(l_sock, iocp);
		}
		break;
		}
	}
failed:
	if (iocp != NULL) {
		CloseHandle(iocp);
	}

	if (l_sock != INVALID_SOCKET) {
		closesocket(l_sock);
	}
	WSACleanup();
	exit_server_gateway();
	printf("server stop!!\n");
}

const char* conver_socket_type_str(int socket_type) {
	if (TCP_SOCKET_IO == socket_type) {
		return  "TCP_SOCKET";
	}
	else if (WEB_SOCKET_IO == socket_type) {
		return "WEB_SOCKET";
	}
	else {
		return "Unkonow";
	}
}

const char* conver_protocal_str(int protocal_type) {
	if (BIN_PROTOCAL == protocal_type) {
		return  "二进制协议";
	}
	else if (JSON_PROTOCAL == protocal_type) {
		return "JSON协议";
	}
	else {
		return "Unkonow";
	}
}