#ifndef QUERY_CALLBACK_H__
#define QUERY_CALLBACK_H__

extern "C" {
#include "../3rd/mjson/json.h"
}
#include <vector>
#include <map>

typedef struct context_req {
	void* mysql_handle;
	json_t* root; //session收到的数据
}context_req;

typedef std::vector<std::vector<std::string> > DBRES;
typedef std::vector<std::map<std::string,std::string> > DBRESMAP;
typedef void(*cb_connect_db)(char* error, void* context,void* udata);
typedef void(*cb_query_db)(char*error, DBRES* res);
typedef void(*cb_close_db)(char*error);
/*
上层请求回调函数定义
查询成功 error==NULL,反之不为NULL，查询错误
res.size()!=0说明查询到结果集，res.size()==0没有查询到记录
res和err参数在底层会被释放，上次要一直使用需要copy一个拷贝
context返回应用层传入的自定义数据
*/
typedef void(*cb_query_db_res_map)(char*error, DBRESMAP* res,void* context);

typedef void(*cb_query_no_res_cb)(char* err);


#endif