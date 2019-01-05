#include "mem_manger.h"

memory_mgr::memory_mgr() {
	_mem32.init_memory();
}

memory_mgr::~memory_mgr() {
}

memory_mgr& memory_mgr::get_instance() {
	static memory_mgr m;
	return m;
}

static void* alloc_default(size_t size) {
	char* pbuf = (char*)malloc(size + sizeof(mem_block));
	if (pbuf==NULL) {
		return NULL;
	}
	mem_block* p = (mem_block*)pbuf;
	p->alloc = NULL;
	p->block_id = -1;
	p->block_ref = 1;
	p->is_pool = false;
	p->next = NULL;
	return pbuf + sizeof(mem_block);
}
void* memory_mgr::alloc_memory(size_t size) {
	void* buf = NULL;
	
	if (size > MAX_ALLOC_SIZE) {
		buf = alloc_default(size);
		return buf;
	}
	
	if (size <= ALLOC_SIZE_32) {
		buf = _mem32.alloc();
	}
	else if (size <= ALLOC_SIZE_64) {
		buf = _mem64.alloc();
	}
	else if (size <= ALLOC_SIZE_128) {
		buf = _mem128.alloc();
	}
	else {
		buf = alloc_default(size);
	}
	return buf;
}

void memory_mgr::free_memory(void* p) {
	if (p != NULL) {
		mem_block* pBlock = (mem_block*)((char*)p - sizeof(mem_block));
		pBlock->block_ref--;
		if (pBlock->block_ref <= 0) {
			if (pBlock->is_pool == false) {
				free(pBlock);
			}
			else {
				if (pBlock->alloc!=NULL) {
					pBlock->alloc->mfree(p);
				}
			}
		}
	}
}
