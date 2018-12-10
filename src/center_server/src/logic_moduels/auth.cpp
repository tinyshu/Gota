#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include "auth.h"

#include "../../../database/query_callback.h"
extern "C" {
#include "../../../types_service.h"
#include "../../../command.h"
#include "../../../error_command.h"
#include "../../../moduel/session/tcp_session.h"
}

#include "../db_moduels/login_moduel.h"
#include "../../../database/center_db.h"
#include "../../database/mysql_warpper.h"

static char sql_buf[1024];
/*
����:
0:�������� stype
1:�������� cmd
2:ukey 
3:unick
4:usex
5:uface

��Ӧ
status,  --OK
unick,
uface,
usex.
*/
/////////////////�첽����mysql��ʽ
//static void on_insert_userinfo(char* err) {
//	if (err != NULL) {
//		printf("%s", err);
//		return;
//	}
//
//}
//
//static void on_query_userinfo(char* err,DBRESMAP* res,void* context) {
//	if (err != NULL) {
//		printf("%s", err);
//		return;
//	}
//	context_req* r_context = (context_req*)context;
//	json_t* root = NULL;
//	if (r_context!=NULL) {
//		root = r_context->root;
//	}
//
//	//û���ղ�ѯ�������
//	if (res==NULL) {
//		//û�в�ѯ����¼����Ҫ�Զ�����һ��
//		char* client_rkeys = json_object_get_string(root, "2");
//		if (NULL == client_rkeys) {
//			//write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client rand key", uid, skey);
//			return;
//		}
//
//		char* user_nick = json_object_get_string(root, "3");
//		if (NULL == user_nick) {
//			//write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client nick name", uid, skey);
//			return;
//		}
//
//		unsigned int user_sex = json_object_get_unsigned_number(root, "4");
//		unsigned int user_face = json_object_get_unsigned_number(root, "5");
//
//		char* sql = "insert into t_user_info(`Frand_key`, `Fnick_name`, \
//`Fuface`, `Fsex`)values(\"%s\", \"%s\", %d, %d)";
//		sprintf(sql_buf, sql, user_nick, user_nick, user_face, user_sex);
//		mysql_wrapper::query_no_res((void*)r_context, sql_buf, on_insert_userinfo);
//		return;
//	}
//	printf("count:%d", res->size());
//	if (res->size() > 1) {
//		return;
//	}
//
//	struct user_info user_info;
//	std::map<std::string, std::string>& row = res->at(0);
//	user_info.is_guest = atoi(row["Fis_guest"].c_str());
//	strncpy(user_info.phone_num, row["Fphone_number"].c_str(), row["Fphone_number"].length());
//	user_info.uface = atoi(row["Fuface"].c_str());
//	user_info.uid = strtoul(row["Fuid"].c_str(), 0, 10);
//	strncpy(user_info.unick, row["Fnick_name"].c_str(), row["Fnick_name"].length());
//	user_info.usex = atoi(row["Fsex"].c_str());
//
//	if (0 == user_info.is_guest) {
//		//�����οͣ����ش�����Ϣ
//		
//		return;
//	}
//
//}
//
//void guest_login(void* moduel_data, struct session* s,
//	json_t* root, unsigned int uid, unsigned int skey) {
//	
//	int para_count = json_object_size(root);
//	if (6+2 != para_count) {
//		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "parament count error", uid, skey);
//		return;
//	}
//
//	char* client_rkeys = json_object_get_string(root,"2");
//	if (NULL == client_rkeys) {
//		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client rand key", uid, skey);
//		return;
//	}
//
//	char* user_nick = json_object_get_string(root, "3");
//	if (NULL == user_nick) {
//		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client nick name", uid, skey);
//		return;
//	}
//
//	unsigned int user_sex = json_object_get_unsigned_number(root, "4");
//	unsigned int user_face = json_object_get_unsigned_number(root, "5");
//	//ʹ��key��db�����
//	//on_query_userinfo();
//	context_req* context = new context_req;
//	context->mysql_handle = mysql_center;
//	
//	context->root = root;
//	char* sql = "select Fuid,Fnick_name,Fis_guest,Fsex,Fuface,Fphone_number from t_user_info where Frand_key=\"%s\" limit 1";
//	snprintf(sql_buf, sizeof(sql_buf), sql, client_rkeys);
//
//	//struct user_info user_info;
//	mysql_wrapper::query2map(context, sql_buf, on_query_userinfo);
//
//	///////////////////////////
//	/*struct user_info user_info;
//	int ret = query_guest_login(client_rkeys,user_nick,user_sex,user_face, &user_info);
//	if (MODEL_ACCOUNT_IS_NOT_GUEST == ret) {
//		write_error(s, SYPTE_CENTER, GUEST_LOGIN, ACCOUNT_IS_NOT_GUEST,"client is not guest", uid, skey);
//		return;
//	}
//
//	if (MODEL_SUCCESS != ret) {
//		write_error(s, SYPTE_CENTER, GUEST_LOGIN, SYSTEM_ERROR, "process guest_login error", uid, skey);
//		return;
//	}
//
//	json_t* send_json = json_new_server_return_cmd(SYPTE_CENTER, GUEST_LOGIN, user_info.uid, skey);
//	json_object_push_number(send_json, "2",OK);
//	json_object_push_string(send_json,"3", user_info.unick);
//	json_object_push_number(send_json,"4",user_info.uface);
//	json_object_push_number(send_json, "5", user_info.usex);
//	
//	session_json_send(s, send_json);
//	json_free_value(&send_json);*/
//
//}
/////////////////

////////////////mysqlͬ�����ʷ�ʽ
void guest_login(void* moduel_data, struct session* s,
	json_t* root, unsigned int uid, unsigned int skey) {

	int para_count = json_object_size(root);
	if (6 + 2 != para_count) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "parament count error", uid, skey);
		return;
	}

	char* client_rkeys = json_object_get_string(root, "2");
	if (NULL == client_rkeys) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client rand key", uid, skey);
		return;
	}

	char* user_nick = json_object_get_string(root, "3");
	if (NULL == user_nick) {
		write_error(s, CENTER_TYPE, GUEST_LOGIN, INVALID_PARAMS, "not fount client nick name", uid, skey);
		return;
	}

	unsigned int user_sex = json_object_get_unsigned_number(root, "4");
	unsigned int user_face = json_object_get_unsigned_number(root, "5");
	//ʹ��key��db�����

	struct user_info user_info;
	int ret = query_guest_login(client_rkeys, user_nick, user_sex, user_face, &user_info);
	if (MODEL_ACCOUNT_IS_NOT_GUEST == ret) {
		write_error(s, SYPTE_CENTER, GUEST_LOGIN, ACCOUNT_IS_NOT_GUEST, "client is not guest", uid, skey);
		return;
	}

	if (MODEL_SUCCESS != ret) {
		write_error(s, SYPTE_CENTER, GUEST_LOGIN, SYSTEM_ERROR, "process guest_login error", uid, skey);
		return;
	}

	json_t* send_json = json_new_server_return_cmd(SYPTE_CENTER, GUEST_LOGIN, user_info.uid, skey);
	json_object_push_number(send_json, "2", OK);
	json_object_push_string(send_json, "3", user_info.unick);
	json_object_push_number(send_json, "4", user_info.uface);
	json_object_push_number(send_json, "5", user_info.usex);

	session_json_send(s, send_json);
	json_free_value(&send_json);

}