#ifndef HASH_MAP_INT_H__
#define HASH_MAP_INT_H__

struct hash_map_int;

extern struct hash_map_int* create_hash_map_int();

extern int set_hash_map_int(struct hash_map_int* map,int key,void* value);

extern void remove_hash_int_by_key(struct hash_map_int* map, int key);

extern void remove_hash_int_by_value(struct hash_map_int* map, void* value);

extern void destory_hash_map(struct hash_map_int* map);

extern void* get_value_by_key(struct hash_map_int* map,int key);
#endif