#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hash_map_int.h"

#define my_malloc malloc
#define my_free free
#define HASH_AREA 1024 

struct hash_node {
	int key;
	void* value;
	struct hash_node* next;
};

struct hash_map_int {
	struct hash_node* hash_set[HASH_AREA];
};

struct hash_map_int* create_hash_map_int() {
	struct hash_map_int* hash_map =(struct hash_map_int*)my_malloc(sizeof(struct hash_map_int));
	memset(hash_map,0,sizeof(struct hash_map_int));
	return hash_map;
}

int set_hash_map_int(struct hash_map_int* map,int key, void* value) {
	int index = key % HASH_AREA;

	struct hash_node** walk = &map->hash_set[index];
	//判断key是否已经存在 
	while (*walk) {
		if ((*walk)->key == key) {
			(*walk)->value = value;
			return 1;
		}
		
		walk = &(*walk)->next;
	}

	struct hash_node* node = (struct hash_node*)my_malloc(sizeof(struct hash_node));
	memset(node, 0, sizeof(struct hash_node));
	node->key = key;
	node->value = value;
	node->next = NULL;
	*walk = node;
	
	//(*walk)->next = node;
}

void remove_hash_int_by_key(struct hash_map_int* map,int key) {
	int index = key % HASH_AREA;
	struct hash_node** walk = &map->hash_set[index];
	while (NULL != *walk) {
		if ((*walk)->key == key) {
			struct hash_node* node = *walk;
			*walk = node->next;
			my_free(node);
			node = NULL;
		}
		walk = &(*walk)->next;
	}
}

void remove_hash_int_by_value(struct hash_map_int* map ,void* value) {
	//删除所有value相等需要遍历全部节点
	for (int i = 0; i < HASH_AREA;++i) {
		struct hash_node** walk = &map->hash_set[i];
		while (*walk) {
			struct hash_node* node =  *walk;
			if (node->value == value) {
				*walk = node->next;
				my_free(node);
				node = NULL;
				continue;
			}
			walk = &(*walk)->next;
		}
	}
}

void destory_hash_map(struct hash_map_int* map) {
	for (int i = 0; i < HASH_AREA; ++i) {
		struct hash_node** walk = &map->hash_set[i];
		while (*walk) {
			struct hash_node* node = *walk;
			*walk = node->next;
			my_free(node);
			node = NULL;
		}
	}

	my_free(map);
}

void* get_value_by_key(struct hash_map_int* map, int key) {
	int index = key % HASH_AREA;
	struct hash_node** walk = &map->hash_set[index];
	while (NULL != *walk) {
		if ((*walk)->key == key) {
			return (*walk)->value;
		}

		walk = &(*walk)->next;
	}
}