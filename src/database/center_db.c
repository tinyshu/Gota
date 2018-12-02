#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "center_db.h"
#include "center_db_conf.h"
#include "../utils/log.h"

MYSQL* mysql_center = NULL;

void connect_to_centerdb() {
	mysql_center = mysql_init(NULL);
	if (NULL == mysql_center) {
		mysql_close(mysql_center);
		exit(-1);
	}

	int is_success = mysql_real_connect(mysql_center, CENTER_DB_CONFIG.mysql_ip, CENTER_DB_CONFIG.mysql_name,
		CENTER_DB_CONFIG.mysql_pwd, CENTER_DB_CONFIG.database_name, CENTER_DB_CONFIG.mysql_port,NULL,0);

	if (0 == is_success) {
		LOGERROR("connect error!!! \n %s\n", mysql_error(mysql_center));
		mysql_close(mysql_center);
		mysql_center = NULL;
		exit(-1);
	}
	else {
		LOGINFO("connect user_center db sucess ip:%s", CENTER_DB_CONFIG.mysql_ip);
	}
}