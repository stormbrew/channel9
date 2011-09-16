#pragma once

#include <set>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

#include <sys/mman.h>

#include "channel9.hpp"
#include "memcheck.h"
#include "bittwiddle.hpp"
#include "forwardtable.hpp"

namespace Channel9
{
	class GC::Markcompact : protected GC
	{
		struct Block;
		struct Chunk;

	public:
		typedef unsigned char uchar;

		struct Data
		{
			uint16_t m_type;
			uint8_t  m_block_size; //so (this & ~((1<<blocksize)-1)) gives the pointer to the block header
			bool     m_mark;
			uint32_t m_count;   //number of bytes of memory in this allocation
			uchar    m_data[0]; //the actual data, 8 byte aligned

			void init(uint16_t type, uint8_t block_size, uint32_t count){
				m_type = type;
				m_block_size = block_size;
				m_mark = false;
				m_count = count;
			}

			uchar * begin() { return m_data; }
			uchar * end()   { return (m_data + m_count); }
			void deadbeef()
			{
				uint32_t * i = (uint32_t *) begin(),
				         * e = (uint32_t *) end();
				for(; i < e; ++i)
					*i = 0xDEADBEEF;
			}

			Data * next() const { return (Data*)((uchar*)(this + 1) + m_count); }
			Block * block() const { return (Block *)((uintptr_t)this & ~((1 << m_block_size)-1)); }
			static Data *ptr_for(const void *ptr) { return (Data*)ptr - 1; }
		};

	private:

		static const double   GC_GROWTH_LIMIT = 2.0;
		static const double   FRAG_LIMIT = 0.8; //compact if the block is less than this full
		static const uint64_t CHUNK_SIZE = 15;
		static const uint64_t BLOCK_SIZE = 12;

		struct Block
		{
			uint32_t m_capacity;   //in bytes, total size not including this header
			uint32_t m_next_alloc; //in bytes, offset to the next allocation
			uint32_t m_in_use;     //in bytes, how much is actually used, is m_next - unmarked Data pieces
			uint8_t  m_skipped;    //how many times a block has skipped compaction
			uint8_t  m_block_size; //used to initialize Data::m_block_size
			bool     m_mark;       //has this anything block been reached yet?
			uint8_t  padding;
			uchar    m_data[0];    //actual memory

			void init(uint8_t block_size)
			{
				m_capacity = (1 << block_size) - sizeof(Block);
				m_next_alloc = 0;
				m_in_use = 0;
				m_skipped = 0;
				m_block_size = block_size;
				m_mark = false;
				DO_DEBUG VALGRIND_CREATE_MEMPOOL(m_data, 0, false);
			}

			Data * alloc(size_t size, uint16_t type)
			{
				size_t alloc_size = size + sizeof(Data);
				if(m_next_alloc + alloc_size <= m_capacity)
				{
					Data * ret = (Data*)(m_data + m_next_alloc);
					ret->init(type, m_block_size, size);
					m_next_alloc += alloc_size;
					DO_DEBUG VALGRIND_MEMPOOL_ALLOC(m_data, ret, size);
					return ret;
				}
				return NULL;
			}
			Data * begin() const { return (Data *) m_data; }
			Data * end()   const { return (Data *) (m_data + m_next_alloc); }
			Block * next() const { return (Block*)((uchar*)(this + 1) + m_capacity); }

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
			uchar *  m_data;     //pointer to actual memory

			void init(uint32_t c, Block * b)
			{
				m_capacity = c;
				m_data = (uchar *) b;
			}

			Block * begin(){ return (Block *) m_data; }
			Block * end()  { return (Block *) (m_data + m_capacity); }
		};



		std::vector<Chunk>   m_chunks;       // list of all chunks
		std::vector<Block *> m_empty_blocks; // list of empty blocks
		Block * m_cur_block;


		ForwardTable forward;

		enum GCPhase { Running, Marking, Compacting, Updating };

		GCPhase  m_gc_phase;
		uint64_t m_alloced;  //how much memory are in all pools (active or not) combined
		uint64_t m_used;     //how much memory is used by data blocks, not including the header
		uint64_t m_data_blocks; //how many data allocations are in the current pool
		uint64_t m_next_gc;  //garbage collect when m_next_gc < m_used
		uint64_t m_dfs_marked;
		uint64_t m_dfs_unmarked;
		uint64_t m_dfs_updated;

		std::set<GCRoot*> m_roots;

		void collect();

		uchar *next(size_t size, uint16_t type, bool new_alloc)
		{
			assert(size < 8000);

			TRACE_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "Alloc %u type %x ... ", (unsigned)size, type);

			size += (8 - size % 8) % 8; //8 byte align

			while(1){
				Data * data = m_cur_block->alloc(size, type);

				TRACE_QUIET_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "from block %p, got %p ... ", m_cur_block, data);

				if(data){
					if(new_alloc)
					{
						m_used += size + sizeof(Data);
						m_data_blocks++;
					}

					TRACE_QUIET_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "alloc return %p\n", data->m_data);

					return data->m_data;
				}

				if(m_empty_blocks.empty())
					alloc_chunk();

				m_cur_block = m_empty_blocks.back();
				m_empty_blocks.pop_back();

				TRACE_QUIET_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "grabbing a new empty block: %p ... ", m_cur_block);
			}
			return NULL;
		}

		void alloc_chunk(unsigned int size = 0){
			if(size)
				size = ceil_power2(size);
			else
				size = 1<<CHUNK_SIZE;

			Block * b = (Block *)mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);

			TRACE_PRINTF(TRACE_ALLOC, TRACE_INFO, "alloc chunk %p, %i zeros\n", b, (int)count_bottom_zeros4((uintptr_t)b));

			assert(count_bottom_zeros4((uintptr_t)b) >= BLOCK_SIZE); //make sure blocks will be properly aligned

			m_alloced += size;

			Chunk c;
			c.init(size, b);

			m_chunks.push_back(c);

			for(Block * i = c.begin(); i != c.end(); i = i->next()){
				assert(i < c.end());
				i->init(BLOCK_SIZE);
				m_empty_blocks.push_back(i);
			}
		}

	public:
		Markcompact()
		 : m_cur_block(NULL), m_gc_phase(Running), m_alloced(0), m_used(0), m_data_blocks(0), m_next_gc(0.9*(1<<CHUNK_SIZE))
		{
			alloc_chunk();

			m_cur_block = m_empty_blocks.back();
			m_empty_blocks.pop_back();
		}

		template <typename tObj>
		tObj *alloc(size_t extra, uint16_t type)
		{
			return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type, true));
		}

		template <typename tObj>
		bool validate(tObj * from)
		{
			if(m_gc_phase != Running)
				return true;
			else
				return (Data::ptr_for(from)->m_mark == false);
		}

		template <typename tObj>
		bool mark(tObj ** from)
		{
			Data * d = Data::ptr_for(*from);
			switch(m_gc_phase)
			{
			case Marking: {

				TRACE_PRINTF(TRACE_GC, TRACE_SPAM, "Marking %p -> %i\n", *from, d->m_mark);

				if(d->m_mark)
					return false;

				m_dfs_marked++;
				d->m_mark = true;

				m_data_blocks++;
				m_used += d->m_count + sizeof(Data);

				Block * b = d->block();
				if(!b->m_mark)
				{
					b->m_mark = true;
					b->m_in_use = 0;
				}
				b->m_in_use += d->m_count + sizeof(Data);

				gc_scan(*from);

				return false;
			}
			case Updating: {
				tObj * to = forward.get(*from);

				TRACE_PRINTF(TRACE_GC, TRACE_SPAM, "Updating %p -> %p", *from, to);

				bool changed = (to != NULL);
				if(changed)
				{
					m_dfs_updated++;
					*from = to;
					d = Data::ptr_for(*from);
				}

				TRACE_QUIET_PRINTF(TRACE_GC, TRACE_SPAM, ", d->m_mark = %i\n", d->m_mark);

				if(d->m_mark)
				{
					m_dfs_unmarked++;
					d->m_mark = false;
					gc_scan(*from);
				}

				return changed;
			}
			case Running:
			case Compacting:
			default:
				assert(false && "Markcompact::mark should only be called when marking or updating");
				return false;
			}
		}

		// make sure this object is ready to be read from
		template <typename tObj>
		void read_barrier(tObj * obj) { }

		// tell the GC that obj will contain a reference to the object pointed to by ptr
		template <typename tObj, typename tPtr>
		void write_barrier(tObj * obj, tPtr * ptr) { }

		// now is a valid time to stop the world
		void safe_point() {
			if(m_next_gc < m_used)
				collect();
		}

		void register_root(GCRoot *root);
		void unregister_root(GCRoot *root);
	};
}

