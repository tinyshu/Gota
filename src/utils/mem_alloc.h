#ifndef MEM_ALLOC_H__
#define MEM_ALLOC_H__
#include <memory>
#include "mem_block.h"
#include <assert.h>

class mem_alloc {
public:
	mem_alloc() {
		_buf = NULL;
		_block_head = NULL;
	}
	~mem_alloc() {
		if (_buf!=NULL) {
			free(_buf);
			_buf = NULL;
		}
	}

	void* alloc() {
		if (_buf==NULL) {
			init_memory();
		}
		char* pbuf = NULL;
		if (_block_head==NULL) {
			//缓存没有可用内存，直接从os申请
			pbuf = (char*)malloc(_block_size+sizeof(mem_block));
			mem_block* p = (mem_block*)pbuf;
			p->alloc = this;
			p->block_id = -1;
			p->block_ref = 1;
			p->is_pool = false;
			p->next = NULL;
		}
		else {
			pbuf = (char*)_block_head;
			_block_head->block_ref = 1;
			_block_head = _block_head->next;
		}
		 
		return pbuf + sizeof(mem_block);
	}

	void  mfree(void* p) {
		mem_block* pblock = (mem_block*)((char*)p-sizeof(mem_block));
		
		if (pblock->is_pool==false) {
			free(pblock);
		}
		else {
			pblock->next = _block_head;
			_block_head = pblock;
		}
	}

	void init_memory() {
		assert(_buf == NULL);
		
		size_t real_size = sizeof(mem_block) + _block_size;
		size_t total_size = real_size * _block_count;
		_buf = (char*)malloc(total_size);
		assert(_buf != NULL);
		
		_block_head = (mem_block*)_buf;
		_block_head->alloc = this;
		_block_head->block_id = 0;
		_block_head->block_ref = 0;
		_block_head->is_pool = true;
		_block_head->next = NULL;
		mem_block* tmp = _block_head;

		for (int i = 1; i < _block_count;++i) {
			mem_block* pblock = (mem_block*)(_buf + (i*real_size));
			pblock->alloc = this;
			pblock->block_id = i;
			pblock->block_ref = 0;
			pblock->is_pool = true;
			pblock->next = NULL;
			tmp->next = pblock;
			tmp = pblock;
		}
	}
private:
	char* _buf;
	mem_block* _block_head;
protected:
	size_t _block_size;
	size_t _block_count;

};

template<size_t nSzie, size_t nBlockSzie>
class memory_alloctor :public mem_alloc
{
public:
	memory_alloctor()
	{
		const size_t n = sizeof(void*);
		_block_size = (nSzie / n)*n + (nSzie % n ? n : 0);
		_block_count = nBlockSzie;
	}

};

#endif