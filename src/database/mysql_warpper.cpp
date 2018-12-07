#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../utils/logger.h"
#include "uv.h"
#include "mysql.h"
extern "C" {
#include "../moduel/net/net_io.h"
}
#include "mysql_warpper.h"

#ifdef WIN32
#include <winsock.h>
#endif

extern uv_loop_t* get_uv_loop();

#define my_malloc malloc 
#define my_free free 

typedef struct connect_req {
	char ip[64];
	int port;
	char db_name[64];
	char uname[64];
	char upasswd[64];
	
	cb_connect_db f_connect_db;
	char* err;
	void* context;

}connect_req;

void on_connecing_cb(uv_work_t* req) {
	printf("connecing db\n");
	connect_req* conn_req = static_cast<connect_req*>(req->data);
	if (conn_req==NULL) {
		return;
	}

	MYSQL* pConn = mysql_init(NULL);
	if (NULL==mysql_real_connect(pConn, conn_req->ip, conn_req->uname,
		conn_req->upasswd, conn_req->db_name, conn_req->port, NULL, 0)){
		log_debug("connect error!!! \n %s\n", mysql_error(pConn));
		conn_req->err = (char*)mysql_error(pConn);
		conn_req->context = NULL;
		mysql_close(pConn);
		pConn = NULL;
		
	}
	else {
		conn_req->context = static_cast<void*>(pConn);
		conn_req->err = NULL;
	}

}

void on_connect_complete_cb(uv_work_t* req, int status) {
	printf("connecing complete db\n");
}

void mysql_wrapper::connect(const char* ip, int port, const char* db_name, const char* user_name,
	const char* passwd, cb_connect_db connect_db) {

	if (ip==NULL || db_name==NULL) {
		log_error("connect db parament is null\n");
		return;
	}

	uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
	if (work==NULL) {
		log_error("connect db malloc uv_work_t error\n");
		return;
	}
	memset(work,0,sizeof(uv_work_t));

	connect_req* conn_req = (connect_req*)malloc(sizeof(connect_req));
	if (conn_req == NULL) {
		log_error("connect db malloc connect_req error\n");
		return;
	}
	memset(conn_req,0,sizeof(connect_req));

	conn_req->port = port;
	conn_req->f_connect_db = connect_db;
	strncpy(conn_req->ip, ip,strlen(ip)+1);
	strncpy(conn_req->db_name, ip, strlen(db_name)+1);
	strncpy(conn_req->uname,user_name,strlen(user_name)+1);
	strncpy(conn_req->upasswd, passwd, strlen(passwd) + 1);

	work->data = static_cast<void*>(conn_req);
	uv_queue_work(get_uv_loop(), work, on_connecing_cb, on_connect_complete_cb);
}

void mysql_wrapper::close(void* context) {

 }

void mysql_wrapper::query(void* context, const char* sql, cb_query_db on_query) {

 }


