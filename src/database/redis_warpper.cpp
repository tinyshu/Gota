#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../utils/logger.h"
#ifdef WIN32	
#define NO_QFORKIMPL //这一行必须加才能正常使用
//#define WIN32_IOCP
#include <Win32_Interop/win32fixes.h>
#pragma comment(lib,"hiredis.lib")
#pragma comment(lib,"Win32_Interop.lib")
#endif

extern "C" {
#include "../moduel/net/net_io.h"
}
//下面2个头文件顺序不能变，否侧包类型测定仪错误
#include "hiredis.h"
#include "uv.h"
#include "redis_warpper.h"

extern uv_loop_t* get_uv_loop();
#define my_malloc malloc 
#define my_free free 

typedef struct redis_connect_req {
	char ip[64];
	int port;
	int timeout;

	cb_connect_db f_connect_db;
	char* err;
	void* context;
	void* u_data;
}redis_connect_req;

typedef struct redis_close_req {
	char* err;
	//lua层传入的连接上下文，实际是redis_lock_context对象指针
	void* context; 
	cb_close_db f_close_cb;
}redis_close_req;

typedef struct redis_query_req {
	char* err;
	void* context; //redis_lock_context对象指针
	char* cmd;
	redisReply* res;
	void* r_context; //这个是应用层传入的上下文,现在存储的是lua的函数handle
	redis_query_cb f_query_cb;
	void* udata;
	
}redis_query_req;

typedef struct redis_lock_context {
	void* connect_handle;
	uv_mutex_t mutex;
}redis_lock_context;

static void on_connect_work_cb(uv_work_t* req) {
	redis_connect_req* conn_req = static_cast<redis_connect_req*>(req->data);
	if (conn_req == NULL) {
		return;
	}
	struct timeval timeout_val;
	timeout_val.tv_sec = 5;
	timeout_val.tv_usec = 0;

	
	redis_lock_context* lock_context = (redis_lock_context*)conn_req->context;
	uv_mutex_lock(&(lock_context->mutex));
	redisContext* rc = redisConnectWithTimeout((char*)conn_req->ip, conn_req->port, timeout_val);
	if (rc->err) {
		printf("Connection error: %s\n", rc->errstr);
		conn_req->err = strdup(rc->errstr);
		conn_req->context = NULL;
		redisFree(rc);
	}
	else {
		
		if (lock_context ==NULL) {
			uv_mutex_unlock(&(lock_context->mutex));
			return;
		}
		lock_context->connect_handle = rc;
		

		conn_req->err = NULL;
		//conn_req->context = (void*)lock_context;
	}

	uv_mutex_unlock(&(lock_context->mutex));
}

static void on_connect_done_cb(uv_work_t* req, int status) {
	redis_connect_req* conn_req = static_cast<redis_connect_req*>(req->data);
	//通知上层回调函数,如果是连接池在这里可以把连接句柄通知到上层
	conn_req->f_connect_db(conn_req->err, conn_req->context, conn_req->u_data);

	if (conn_req->err!=NULL) {
		my_free(conn_req->err);
		conn_req->err = NULL;
	}

	my_free(conn_req);
	my_free(req);
}

void redis_wrapper::rediseconnect(const char* ip, int port, int timeout, cb_connect_db connect_db, void* udata) {
	uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("connect db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	redis_connect_req* conn_req = (redis_connect_req*)malloc(sizeof(redis_connect_req));
	if (conn_req == NULL) {
		log_error("connect db malloc connect_req error\n");
		return;
	}
	memset(conn_req, 0, sizeof(redis_connect_req));
	
	redis_lock_context* lock_context = (redis_lock_context*)my_malloc(sizeof(redis_lock_context));
	lock_context->connect_handle = NULL;
	uv_mutex_init(&(lock_context->mutex));

	conn_req->port = port;
	conn_req->f_connect_db = connect_db;
	strncpy(conn_req->ip, ip, strlen(ip) + 1);
	conn_req->u_data = udata; //存储lua函数handleid
	conn_req->context = lock_context;
	work->data = static_cast<void*>(conn_req);

	uv_queue_work(get_uv_loop(), work, on_connect_work_cb, on_connect_done_cb);
}

static void on_close_work_cb(uv_work_t* req) {
	if (req == NULL) {
		return;
	}

	redis_close_req* r = (redis_close_req*)(req->data);
	if (r==NULL) {
		return;
	}

	redis_lock_context* lock_context = (redis_lock_context*)r->context;
	redisContext* r_context = (redisContext*)r->context;
	
	uv_mutex_lock(&(lock_context->mutex));
	redisFree(r_context);
	uv_mutex_unlock(&(lock_context->mutex));
}

static void on_close_done_cb(uv_work_t* req,int status) {
	redis_close_req* r = (redis_close_req*)(req->data);
	if (r == NULL) {
		return;
	}

	if(r->f_close_cb!=NULL){
	   r->f_close_cb(r->err);
	}
	
	if (r->err!=NULL) {
		my_free(r->err);
	}

	if (r->context != NULL) {
		my_free(r->context);
	}

	my_free(r);
	my_free(req);
}

void redis_wrapper::rediseclose(void* context, cb_close_db on_close) {
	uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("connect db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	redis_close_req* r = (redis_close_req*)malloc(sizeof(redis_close_req));
	if (r == NULL) {
		log_error("connect db malloc connect_req error\n");
		return;
	}
	memset(r, 0, sizeof(redis_connect_req));
	r->err = NULL;
	r->f_close_cb = on_close;
	r->context = context;

	uv_queue_work(get_uv_loop(), work, on_close_work_cb, on_close_done_cb);
}


static void on_query_work_cb(uv_work_t* req) {
	redis_query_req* r = (redis_query_req*)req->data;
	redis_lock_context* lock_context = (redis_lock_context*)r->context;

	uv_mutex_lock(&(lock_context->mutex));
	//reply也需要释放
	redisReply* reply = (redisReply*)redisCommand((redisContext*)lock_context->connect_handle,r->cmd);
	if(reply!=NULL){
		r->res = reply;
	}

	uv_mutex_unlock(&(lock_context->mutex));
}

static void on_query_done_cb(uv_work_t* req,int status) {
	
	redis_query_req* r = (redis_query_req*)req->data;
	if (r->f_query_cb!=NULL) {
		r->f_query_cb(r->err,r->res,r->udata);
	}

	if (r->res!=NULL) {
		freeReplyObject(r->res);
	}
	if (r->cmd != NULL) {
		my_free(r->cmd);
	}

	my_free(r);
	my_free(req);
}

void redis_wrapper::redisequery(void* context, const char* cmd, redis_query_cb f_query_cb, void* udata) {
	if (context==NULL || cmd==NULL) {
		return;
	}
	uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("redisequery db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	redis_query_req* r = (redis_query_req*)malloc(sizeof(redis_query_req));
	if (r == NULL) {
		log_error("redisequery db malloc connect_req error\n");
		return;
	}
	memset(r, 0, sizeof(redis_query_req));

	r->cmd = strdup(cmd);
	r->context = context;
	r->r_context = NULL;
	r->err = NULL;
	r->f_query_cb = f_query_cb;
	r->res = NULL;
	r->udata = udata;
	work->data = r;
	uv_queue_work(get_uv_loop(), work, on_query_work_cb, on_query_done_cb);
}