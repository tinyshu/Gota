#ifndef MEM_BLOCK_H__
#define MEM_BLOCK_H__

class mem_alloc;
typedef struct mem_block {
	int block_id;
	int block_ref;
	bool is_pool;
	mem_block* next;
	mem_alloc* alloc;
}mem_block;

#define MEMBLOCK_SIZE sizeof(mem_block)

#endif