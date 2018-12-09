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

typedef struct close_req{
	char* err;
	void* context;
	cb_close_db f_close_cb;
}close_req;

typedef struct query_req {
	char* err;
	void* context; //这个是mysql句柄
	char* sql;
	DBRES* res;
	DBRESMAP* resmap;
	void* r_context; //这个是应用层传入的上下文
	cb_query_db f_query_cb; //结果集返回vector<vector>
	cb_query_db_res_map f_query_map_cb;  //结果集返回vector<map>
	cb_query_no_res_cb f_query_no_res;   //无结果集返回
}query_req;


void on_connect_work_cb(uv_work_t* req) {
	printf("connecing db\n");
	connect_req* conn_req = static_cast<connect_req*>(req->data);
	if (conn_req==NULL) {
		return;
	}

	MYSQL*pConn = mysql_init(NULL);
	if (NULL==mysql_real_connect(pConn, conn_req->ip, conn_req->uname,
		conn_req->upasswd, conn_req->db_name, conn_req->port, NULL, 0)){
		log_debug("connect error!!! \n %s\n", mysql_error(pConn));
		conn_req->err = strdup(mysql_error(pConn));
		conn_req->context = NULL;
		mysql_close(pConn);
		pConn = NULL;
		
	}
	else {
		conn_req->context = static_cast<void*>(pConn);
		conn_req->err = NULL;
	}

}

void on_connect_done_cb(uv_work_t* req, int status) {
	printf("connecing complete db\n");
	connect_req* conn_req = static_cast<connect_req*>(req->data);
	//通知上层回调函数,如果是连接池在这里可以把连接句柄通知到上层
	conn_req->f_connect_db(conn_req->err, conn_req->context);
	if (conn_req->err!=NULL) {
		//这里要释放strdup获取的char*
		my_free(conn_req->err);
		conn_req->err = NULL;
	}
	my_free(conn_req);
	my_free(req);
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
	strncpy(conn_req->db_name, db_name, strlen(db_name)+1);
	strncpy(conn_req->uname,user_name,strlen(user_name)+1);
	strncpy(conn_req->upasswd, passwd, strlen(passwd) + 1);

	//携带自己的自定义数据
	work->data = static_cast<void*>(conn_req);
	/*
	uv_queue_work添加uv_work_t到线程队列
	on_connect_work_cb 线程调度入口函数
	on_connect_done_cb ,待on_connect_work_cb执行完成调用
	*/
	uv_queue_work(get_uv_loop(), work, on_connect_work_cb, on_connect_done_cb);
}

void on_close_work_cb(uv_work_t* req){
	close_req* cl_req = static_cast<close_req*>(req->data);
	MYSQL* pconn = static_cast<MYSQL*>(cl_req->context);
	if (pconn!=NULL) {
		mysql_close(pconn);
		pconn = NULL;
	}
}

void on_close_done_cb(uv_work_t* req, int status) {
	close_req* cl_req = static_cast<close_req*>(req->data);
	if (cl_req->f_close_cb!=NULL) {
		cl_req->f_close_cb(cl_req->err);
	}
	if (cl_req->err!=NULL) {
		my_free(cl_req->err);
	}
	my_free(cl_req);
	my_free(req);
}

void mysql_wrapper::close(void* context, cb_close_db on_close) {
	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("close db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	close_req* req = (close_req*)my_malloc(sizeof(close_req));
	req->err = NULL;
	req->context = context;
	req->f_close_cb = on_close;
	work->data = static_cast<void*>(req);
	uv_queue_work(get_uv_loop(), work, on_close_work_cb, on_close_done_cb);
 }


void on_query_work_cb(uv_work_t* req) {
	query_req* q_req = static_cast<query_req*>(req->data);
	MYSQL* mysql_handle = static_cast<MYSQL*>(q_req->context);
	
	if (mysql_query(mysql_handle, q_req->sql)) {
		log_error("%s %s\n", q_req->sql, mysql_error(mysql_handle));
		q_req->err = strdup(mysql_error(mysql_handle));
		if (mysql_errno(mysql_handle) == 2006 || mysql_errno(mysql_handle) == 2013) {
			//重连
			//mysql_ping(mysql_handle);
			//continue;
		}

		return;
	}
	//
	//获取结果集
	MYSQL_RES* res = mysql_store_result(mysql_handle);
	if (res == NULL) {
		q_req->err = strdup("get store is null");
		q_req->f_query_cb(q_req->err, NULL);
		return;
	}
	if (mysql_num_rows(res) == 0) {
		q_req->err = strdup("get store num is zero!");
		q_req->f_query_cb(q_req->err, NULL);
		mysql_free_result(res);
		return;
	}
	
	int fields_num = mysql_field_count(mysql_handle);
	if (fields_num<=0) {
		mysql_free_result(res);
		return;
	}
	//mysql_fetch_field
	DBRES* db_res = new DBRES;
	db_res->clear();
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != NULL) {
		std::vector<std::string> d_row;
		d_row.reserve(fields_num);
		for (int i = 0; i < fields_num;i++) {
			if (row[i]==NULL) {
				d_row.push_back("");
			}
			else {
				d_row.push_back(row[i]);
			}
		}

		db_res->push_back(d_row);
	}
	q_req->res = db_res;
	mysql_free_result(res);
}

void on_query_done_cb(uv_work_t* req,int status) {
	query_req* q_req = static_cast<query_req*>(req->data);
	if (q_req->f_query_cb!=NULL) {
		//包查询的结果集回调给上层的应用
		q_req->f_query_map_cb(q_req->err,q_req->resmap, q_req->r_context);
	}

	if (q_req->err!=NULL) {
		free(q_req->err);
		q_req->err = NULL;
	}

	if (q_req->sql!=NULL) {
		free(q_req->sql);
		q_req->sql = NULL;
	}

	if (q_req->res!=NULL) {
		delete q_req->res;
		q_req->res = NULL;
	}

	my_free(q_req);
	my_free(req);
}

void on_query_map_work_cb(uv_work_t* req) {
	query_req* q_req = static_cast<query_req*>(req->data);
	MYSQL* mysql_handle = static_cast<MYSQL*>(q_req->context);

	if (mysql_query(mysql_handle, q_req->sql)) {
		log_error("%s %s\n", q_req->sql, mysql_error(mysql_handle));
		q_req->err = q_req->sql, mysql_error(mysql_handle);
		if (mysql_errno(mysql_handle) == 2006 || mysql_errno(mysql_handle) == 2013) {
			//重连
			//mysql_ping(mysql_handle);
			//continue;
		}

		return;
	}

	//获取结果集
	MYSQL_RES* res = mysql_store_result(mysql_handle);
	if (res == NULL) {
		q_req->err = strdup("get store is null");
		q_req->f_query_map_cb(q_req->err, NULL, q_req->r_context);
		return;
	}
	if (mysql_num_rows(res) == 0) {
		q_req->f_query_map_cb(q_req->err, NULL,q_req->r_context);
		mysql_free_result(res);
		return;
	}

	int fields_num = mysql_field_count(mysql_handle);
	if (fields_num <= 0) {
		mysql_free_result(res);
		return;
	}

	std::vector<std::string> fileld_names;
	MYSQL_FIELD *field=NULL;
	while ((field = mysql_fetch_field(res)))
	{
		fileld_names.push_back(field->name);
	}
	
	DBRESMAP* db_res = new DBRESMAP;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != NULL) {
		//std::vector<std::string> d_row;
		std::map<std::string,std::string> d_row;
		
		for (int i = 0; i < fields_num; i++) {
			if (row[i] == NULL) {
				
				d_row.insert(std::make_pair(fileld_names.at(i),""));
			}
			else {
				
				d_row.insert(std::make_pair(fileld_names.at(i), row[i]));
			}
		}
	
		db_res->push_back(d_row);
	}
	q_req->resmap = db_res;
	mysql_free_result(res);
}

void mysql_wrapper::query2map(void* context, const char* sql, cb_query_db_res_map on_query_map) {
	if (context == NULL || sql == NULL) {
		return;
	}

	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("query db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));
	query_req* req = (query_req*)my_malloc(sizeof(query_req));
	if (req == NULL) {
		log_error("query db malloc query_req error\n");
		return;
	}
	context_req* r_context = (context_req*)context;
	req->context = r_context->mysql_handle;
	req->sql = strdup(sql); //在回调完成后要释放
	req->f_query_map_cb = on_query_map;
	req->err = NULL;
	req->res = NULL;
	req->r_context = r_context;
	work->data = static_cast<void*>(req);
	uv_queue_work(get_uv_loop(), work, on_query_map_work_cb, on_query_done_cb);

}

void mysql_wrapper::query(void* context,const char* sql, cb_query_db on_query) {
	if (context==NULL ||sql==NULL) {
		return;
	}

	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("query db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	query_req* req = (query_req*)my_malloc(sizeof(query_req));
	if (req==NULL) {
		log_error("query db malloc query_req error\n");
		return;
	}
	req->context = context;
	req->sql = strdup(sql); //在回调完成后要释放
	req->f_query_cb = on_query;
	req->err = NULL;
	req->res = NULL;
	work->data = static_cast<void*>(req);
	uv_queue_work(get_uv_loop(), work, on_query_work_cb, on_query_done_cb);
 }

void on_query_done_no_res_cb(uv_work_t* req, int status) {
	query_req* q_req = static_cast<query_req*>(req->data);
	if (q_req->f_query_no_res!=NULL) {
		q_req->f_query_no_res(q_req->err);
	}
}

void on_query_work_no_res_cb(uv_work_t* req) {
	query_req* q_req = static_cast<query_req*>(req->data);
	MYSQL* mysql_handle = static_cast<MYSQL*>(q_req->context);

	if (mysql_query(mysql_handle, q_req->sql)) {
		q_req->err = strdup(mysql_error(mysql_handle));
		return;
	}
}

void mysql_wrapper::query_no_res(void* context, const char* sql, cb_query_no_res_cb on_query_no_res_cb) {
	if (context == NULL || sql == NULL) {
		return;
	}

	uv_work_t* work = (uv_work_t*)my_malloc(sizeof(uv_work_t));
	if (work == NULL) {
		log_error("query db malloc uv_work_t error\n");
		return;
	}
	memset(work, 0, sizeof(uv_work_t));

	query_req* req = (query_req*)my_malloc(sizeof(query_req));
	if (req == NULL) {
		log_error("query db malloc query_req error\n");
		return;
	}
	
	/*
	context_req* r_context = (context_req*)context;
	req->context = r_context->mysql_handle;
	req->sql = strdup(sql); //在回调完成后要释放
	req->f_query_map_cb = on_query_map;
	req->err = NULL;
	req->res = NULL;
	req->r_context = r_context;
	*/

	req->context = context;
	req->sql = strdup(sql); //在回调完成后要释放
	req->f_query_no_res = on_query_no_res_cb;
	req->err = NULL;
	req->res = NULL;
	work->data = static_cast<void*>(req);

	uv_queue_work(get_uv_loop(), work, on_query_work_no_res_cb, on_query_done_no_res_cb);

}
