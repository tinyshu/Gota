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

void* memory_mgr::alloc_memory(size_t size) {
	void* buf = NULL;
	if (size <= ALLOC_SIZE_32) {
		buf = _mem32.alloc();
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
