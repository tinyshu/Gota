#ifndef SESSION_KEY_MGR_H__
#define SESSION_KEY_MGR_H__

//从from_client转发到后端服务，会生成一个key和client的session对应
//这样在return_server发回给client可以使用这个key找到session


void init_session_key_map();

void exit_session_key_map();

unsigned int get_session_key();

void save_session_by_key(unsigned int key,struct session* s);

void remove_session_by_key(unsigned int key);

void clear_session_by_value(struct session* s);

struct session* get_session_by_key(unsigned int key);

#endif 