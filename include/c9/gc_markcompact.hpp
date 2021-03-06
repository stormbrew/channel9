#pragma once

#include <set>
#include <vector>
#include <deque>
#include <stack>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

#include <sys/mman.h>

#include "c9/channel9.hpp"
#include "c9/bittwiddle.hpp"
#include "c9/forwardtable.hpp"

#include "memcheck.h"

namespace Channel9
{
	class GC::Markcompact : public GC
	{
		struct Block;
		struct Chunk;

	public:
		struct Data
		{
			uint32_t m_count;   //number of bytes of memory in this allocation
			uint8_t m_generation; // the generation. Always NORMAL.
			uint8_t m_type;
			uint8_t  m_block_size; //so (this & ~((1<<blocksize)-1)) gives the pointer to the block header
			unsigned m_mark   : 1; //is this marked
			unsigned m_pinned : 1; //is this pinned in memory
			unsigned m_padding : 6;//padding for 8 byte alignment
			uint8_t  m_data[0]; //the actual data, 8 byte aligned

			void init(uint32_t count, uint8_t type, uint8_t block_size, bool pinned){
				m_count  = count;
				m_generation = GEN_TENURE;
				m_type   = type;
				m_block_size = block_size;
				m_mark   = false;
				m_pinned = pinned;
			}

			uint8_t * begin() { return m_data; }
			uint8_t * end()   { return (m_data + m_count); }
			void deadbeef()
			{
				uint32_t * i = (uint32_t *) begin(),
				         * e = (uint32_t *) end();
				for(; i < e; ++i)
					*i = 0xDEADBEEF;
			}

			Data * next() const { return (Data*)((uint8_t*)(this + 1) + m_count); }
			Block * block() const {
				if(m_pinned) return NULL;
				return (Block *)(floor_power2((uintptr_t)this, m_block_size));
			}
			static Data *ptr_for(const void *ptr) { return (Data*)ptr - 1; }
		};

	private:

		static const uint64_t GC_GROWTH_LIMIT = 2;
		static const uint64_t FRAG_LIMIT = 80; //compact if the block is less than this full. Expressed as integer percentage (80 is 80% full)
		static const uint64_t CHUNK_SIZE = 20; //how much to allocate at a time
		static const uint64_t BLOCK_SIZE = 15; //prefer block alignment over page alignment
		static const uint64_t PAGE_SIZE = 12;  //linux defines 4kb pages, so that's what's used here as mmap guarantees page alignment

		struct Block
		{
			uint32_t m_capacity;   //in bytes, total size not including this header
			uint32_t m_next_alloc; //in bytes, offset to the next allocation
			uint32_t m_in_use;     //in bytes, how much is actually used, is m_next_alloc - unmarked Data pieces
			uint8_t  m_skipped;    //how many times a block has skipped compaction
			uint8_t  m_block_size; //used to initialize Data::m_block_size
			bool     m_mark;       //has this block been reached yet?
			uint8_t  padding;
			uint8_t  m_data[0];    //actual memory

			void init(uint8_t block_size)
			{
				m_capacity = (1 << block_size) - sizeof(Block);
				m_next_alloc = 0;
				m_in_use = 0;
				m_skipped = 0;
				m_block_size = block_size;
				m_mark = false;
				VALGRIND_CREATE_MEMPOOL(m_data, 0, false);
			}

			Data * alloc(size_t size, uint16_t type)
			{
				size_t alloc_size = size + sizeof(Data);
				if(m_next_alloc + alloc_size <= m_capacity)
				{
					Data * ret = (Data*)(m_data + m_next_alloc);
					VALGRIND_MEMPOOL_ALLOC(m_data, ret, alloc_size);
					ret->init(size, type, m_block_size, false);
					m_next_alloc += alloc_size;
					return ret;
				}
				return NULL;
			}
			Data * begin() const { return (Data *) m_data; }
			Data * end()   const { return (Data *) (m_data + m_next_alloc); }
			Block * next() const { return (Block*)((uint8_t*)(this + 1) + m_capacity); }

			void deadbeef()
			{
				uint32_t * i = (uint32_t *) m_data,
				         * e = (uint32_t *) (m_data + m_capacity);
				for(; i < e; ++i)
					*i = 0xDEADBEEF;
			}
		};


		struct Chunk
		{
			uint32_t m_capacity; //in bytes
			uint8_t *  m_data;     //pointer to actual memory

			void init(uint32_t c, Block * b)
			{
				m_capacity = c;
				m_data = (uint8_t *) b;
			}

			Block * begin(){ return (Block *) m_data; }
			Block * end()  { return (Block *) (m_data + m_capacity); }
		};



		std::deque<Chunk>   m_chunks;       // list of all chunks
		std::deque<Block *> m_empty_blocks; // list of empty blocks
		Block * m_cur_medium_block;//block to allocate medium objects out of, moved to small once if it fails to allocate a medium object
		Block * m_cur_small_block; //block to allocate small objects out of, may point to the same as medium

		ForwardTable forward;
		std::stack<Data *>  m_scan_list;  // list of live objects left to scan

		enum GCPhase { Running, Marking, Compacting, Updating };

		GCPhase  m_gc_phase;
		uint64_t m_alloced;  //how much memory are in all pools (active or not) combined
		uint64_t m_used;     //how much memory is used by data blocks, not including the header
		uint64_t m_data_blocks; //how many data allocations are in the current pool
		uint64_t m_next_gc;  //garbage collect when m_next_gc < m_used
		uint64_t m_dfs_marked;
		uint64_t m_dfs_unmarked;
		uint64_t m_dfs_updated;

		std::vector<Data *> m_pinned_objs;
		Block m_pinned_block;

		uint8_t *next_slow(size_t alloc_size, size_t size, uint16_t type, bool new_alloc, bool small);
		uint8_t *next(size_t size, uint16_t type, bool new_alloc){ return next(size, type, new_alloc, (size <= SMALL)); }
		uint8_t *next(size_t size, uint16_t type, bool new_alloc, bool small)
		{
			size = ceil_power2(size, 3); //8 byte align
			size_t alloc_size = size + sizeof(Data);

			if(new_alloc)
			{
				m_used += alloc_size;
				m_data_blocks++;
			}

			Block * block = (small ? m_cur_small_block : m_cur_medium_block);
			Data * data = block->alloc(size, type);

			if(data){
				TRACE_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "Alloc %u type %x ... from block %p, got %p ... alloc return %p\n", (unsigned)size, type, block, data, data->m_data);

				return data->m_data;
			} else {
				return next_slow(alloc_size, size, type, new_alloc, small);
			}
			return NULL;
		}

		void alloc_chunk(){

			//allocate a bit extra because we're going to align to block size and then unmap the left overs
			size_t chunk_size = (1 << CHUNK_SIZE);
			size_t alloc_size = chunk_size + (1 << BLOCK_SIZE);

			char * p = (char *)mmap(NULL, alloc_size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
			char * pe = p + alloc_size;

			char * a = (char *)ceil_power2((uintptr_t)p, BLOCK_SIZE); //where does the alignment start
			char * ae = a + chunk_size;

			if(p != a)
				munmap(p, a - p); //unmap the memory before the block alignment
			munmap(ae, pe - ae);  //unmap the memory after the chunk ends

			Block * b = (Block *)a;
			assert(b != NULL);

			TRACE_PRINTF(TRACE_ALLOC, TRACE_INFO, "alloc chunk %p, %i zeros\n", b, (int)count_bottom_zeros4((uintptr_t)b));

			assert(count_bottom_zeros4((uintptr_t)b) >= BLOCK_SIZE); //make sure blocks will be properly aligned

			m_alloced += chunk_size;

			Chunk c;
			c.init(chunk_size, b);

			m_chunks.push_back(c);

			for(Block * i = c.begin(); i != c.end(); i = i->next()){
				assert(i < c.end());
				assert(count_bottom_zeros4((uintptr_t)i) >= BLOCK_SIZE);

				i->init(BLOCK_SIZE);
				m_empty_blocks.push_back(i);
			}
		}

	public:
		Markcompact();

		// called by the next generation down to promote an
		// already allocated object to the generation in question.
		inline void *promote(void *obj, size_t size, ValueType type, bool pinned)
		{
			void *n = raw_alloc(size, type, pinned);
			memcpy(n, obj, size);
			return n;
		}
		// called by the next generation down to allocate an
		// object is the current generation can't.
		inline void *raw_alloc(size_t size, ValueType type, bool pinned)
		{
			if(pinned)
				alloc_pinned(size, type);

			if(size <= SMALL)
				return next(size, type, true, true);
			else if(size <= MEDIUM)
				return next(size, type, true, false);
			else
				return alloc_pinned(size, type);
		}
		void *alloc_pinned(size_t extra, uint16_t type);

		template <typename tObj>
		bool validate(tObj * from)
		{
			if(m_gc_phase != Running)
				return true;
			else
				return (Data::ptr_for(from)->m_mark == false);
		}

		bool mark(void *obj, uintptr_t *from);

		void scan(Data * d)
		{
			GC::scan(d->m_data, ValueType(d->m_type));
		}

		// make sure this object is ready to be read from
		template <typename tObj>
		void read_ptr(tObj * obj) { }

		template <typename tField, typename tVal>
		void write_ptr(void *obj, tField &field, const tVal &val)
		{
		}

		void collect();
		bool need_collect() const
		{
			return m_next_gc < m_used;
		}
	};

	inline void *GC::Markcompact::alloc_pinned(size_t size, uint16_t type)
	{
		size += (8 - size % 8) % 8; //8 byte align

		Data * data = (Data*) malloc(size + sizeof(Data));
		data->init(size, type, 0, true);
		m_pinned_objs.push_back(data);
		m_pinned_block.m_capacity += size + sizeof(Data);

		m_used += size;
		m_data_blocks++;

		return data->m_data;
	}

}

