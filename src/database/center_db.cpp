#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mysql.h"
#include "center_db.h"
extern "C" {
#include "center_db_conf.h"
#include "../utils/log.h"
}


MYSQL* mysql_center = NULL;

static char sql_buf[2048];

void connect_to_centerdb() {
	if (mysql_center != NULL) {
		mysql_close(mysql_center);
	}
	mysql_center = mysql_init(NULL);
	if (NULL == mysql_center) {
		mysql_close(mysql_center);
		exit(-1);
	}

	if (NULL==mysql_real_connect(mysql_center, CENTER_DB_CONFIG.mysql_ip, CENTER_DB_CONFIG.mysql_name,
		CENTER_DB_CONFIG.mysql_pwd, CENTER_DB_CONFIG.database_name, CENTER_DB_CONFIG.mysql_port, NULL, 0)) {
		LOGERROR("connect error!!! \n %s\n", mysql_error(mysql_center));
		mysql_close(mysql_center);
		mysql_center = NULL;
		exit(-1);
	}
	else {
		LOGINFO("connect user_center db sucess ip:%s", CENTER_DB_CONFIG.mysql_ip);
	}
}

int get_userinfo_buy_key(const char* rand_key, struct user_info* userinfo) {
	if (NULL == mysql_center) {
		connect_to_centerdb();
	}
	
	char* sql = "select Fuid,Fnick_name,Fis_guest,Fsex,Fuface,Fphone_number from t_user_info where Frand_key=\"%s\" limit 1";
	snprintf(sql_buf, sizeof(sql_buf), sql, rand_key);

	LOGINFO("%s", sql_buf);
	int tyr_count = 0;
	while (tyr_count < 2) {
		if (mysql_query(mysql_center, sql_buf)) {
			LOGERROR("%s %s\n", sql_buf, mysql_error(mysql_center));
			if (mysql_errno(mysql_center) == 2006 || mysql_errno(mysql_center) == 2013) {
				tyr_count++;
				connect_to_centerdb();
				continue;
			}
			else {
				return -1;
			}
			
		}
		break;
	}
	

	//获取结果集
	MYSQL_RES* res = mysql_store_result(mysql_center);
	if (res==NULL) {
		return 0;
	}
	if (mysql_num_rows(res) == 0){
		mysql_free_result(res);
		return -1;
	}

	MYSQL_ROW row = mysql_fetch_row(res);
	int fields_num = mysql_field_count(mysql_center);
	if (row) {
		userinfo->uid = atoi(row[0]);
		if(NULL!= row[1]){
			strcpy(userinfo->unick, row[1]);
		}
		
		userinfo->is_guest = atoi(row[2]);
		userinfo->usex = atoi(row[3]);
		userinfo->uface = atoi(row[4]);
		if (NULL!= row[5]) {
			strcpy(userinfo->phone_num, row[5]);
		}
		
	}

	mysql_free_result(res);
	return 0;
}

int insert_guest_with_key(char* ukey, char* unick, int uface, int usex) {
	if (mysql_center == NULL) {
		return -1;
	}
	char* sql = "insert into t_user_info(`Frand_key`, `Fnick_name`, \
`Fuface`, `Fsex`)values(\"%s\", \"%s\", %d, %d)";
	sprintf(sql_buf, sql, ukey, unick, uface, usex);

	if (mysql_query(mysql_center, sql_buf)) {
		LOGERROR("%s %s\n", sql, mysql_error(mysql_center));
		return -1;
	}

	return 0;

}