#ifndef MEM_MANGER_H__
#define MEM_MANGER_H__

#include "mem_alloc.h"

#define ALLOC_SIZE_32 32
#define ALLOC_SIZE_64 64
#define MAX_ALLOC_SIZE 128

class memory_mgr {
	memory_mgr();
	~memory_mgr();
public:
	static memory_mgr& get_instance();

	void* alloc_memory(size_t size);
	void  free_memory(void* p);
private:
	//可以根据业务场景配置多个大小的内存池
	memory_alloctor<ALLOC_SIZE_32, 100000> _mem32;
	//mem_alloc<64, 100000> _mem64;
	//mem_alloc<128, 100000> _mem128;	
};

#endif