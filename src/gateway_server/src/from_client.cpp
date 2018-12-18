#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "from_client.h"
#include "../../types_service.h"
#include "../../command.h"
#include "../../moduel/netbus/netbus.h"
#include "server_session_mgr.h"
#include "../../3rd/mjson/json_extends.h"
#include "../../moduel/netbus/session_key_mgr.h"

#define my_malloc malloc
#define my_free free
//�յ��ͻ���json���ݺ�,��������service_gatewayģ��on_json_protocal_data,
//service_gateway�ڸ���stype���õ�����
//�ú���������Ϣ��ת������Ӧ�ĺ�˷���
static int on_json_protocal_data (void* moduel_data, struct session* s, json_t* root, 
	unsigned char* pkg, int len){
	int stype = (int)(moduel_data);
	
	struct session* server_session = get_server_session(stype);
	if (NULL == server_session) {
		//stype����ȷ���������ӵĺ�˷���Ͽ�������
		return 0;
	}

	//��ȡ�û�uid����Ȩ����֤
	unsigned int uid = 123;
	//��ȡһ�����key��session��
	unsigned int session_key = get_session_key();
	save_session_by_key(session_key,s);
	json_object_push_number(root,"uid",uid);
	json_object_push_number(root,"skey", session_key); //��˷�����Ҫ͸���������ֵ
	session_json_send(server_session, root);
	return 0;
}

//������ײ��⵽�ͻ������ӵ�session�رգ��������֪ͨ���ú���
static void on_connect_lost (void* moduel_data, struct session* s) {
	if (1 == s->is_server_session) {
		//������������session����
		return;
	}
	int stype = (int)moduel_data;
	//��Ҫ֪ͨ��˵ķ���,�пͻ���session�Ͽ�
	unsigned int uid = s->uid;
	
	struct session* server_session = get_server_session(stype);
	if (server_session == NULL) {
		//����һ��client���͵�session
		return;
	}
	json_t* json = json_new_comand(stype, USER_LOST_CONNECT);
	json_object_push_number(json, "2", uid);

	session_json_send(server_session, json);
	json_free_value(&json);

	//���hash_map_int������sessionָ��
	clear_session_by_value(s); 
	
}

void register_from_client_moduel(int stype) {
	struct service_module* register_moduel = (struct service_module*)my_malloc(sizeof(struct service_module));
	if (NULL == register_moduel) {
		exit(-1);
	}

	register_moduel->init_service_module = NULL;
	register_moduel->on_bin_protocal_data = NULL;
	register_moduel->on_json_protocal_data = on_json_protocal_data;
	register_moduel->on_connect_lost = on_connect_lost;
	register_moduel->moduel_data = (void*)stype;

	register_services(stype, register_moduel);
}