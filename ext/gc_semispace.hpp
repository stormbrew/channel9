#pragma once

#include <set>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

#include "channel9.hpp"
#include "memcheck.h"

namespace Channel9
{
	class GC::Semispace : protected GC
	{
	public:
		typedef unsigned char uchar;

		struct Data
		{
			uint16_t m_type;
			uint8_t  m_forward; //forwarding pointer?
			uint8_t  m_pool; //which pool
			uint32_t m_count; //number of bytes of memory in this allocation
			uchar    m_data[0]; //the actual data, 8 byte aligned

			Data *next() const { return (Data*)((uchar*)(this + 1) + m_count); }
			static Data *ptr_for(const void *ptr) { return (Data*)ptr - 1; }
		};

	private:

		static const double   GC_GROWTH_LIMIT = 2.0;
		static const uint64_t CHUNK_SIZE = 2<<20; // 2mb

		struct Chunk
		{
			Chunk *  m_next; //linked list of chunks
			uint32_t m_capacity; //in bytes
			uint32_t m_used;     //in bytes
			uchar    m_data[0];  //actual memory

			void init(uint32_t capacity)
			{
				m_next = NULL;
				m_capacity = capacity;
				m_used = 0;
				DO_DEBUG VALGRIND_CREATE_MEMPOOL(m_data, 0, false);
			}

			Data * alloc(size_t size)
			{
				if(m_used + size <= m_capacity)
				{
					Data * ret = (Data*)(m_data + m_used);
					m_used += size;
					DO_DEBUG VALGRIND_MEMPOOL_ALLOC(m_data, ret, size);
					return ret;
				}
				return NULL;
			}
			Data * begin() const { return (Data *) m_data; }
			Data * end()   const { return (Data *) (m_data + m_used); }
			void deadbeef()
			{
				uint32_t * i = (uint32_t *) m_data,
				         * e = (uint32_t *) (m_data + m_capacity);
				for(; i < e; ++i)
					*i = 0xDEADBEEF;
			}
		};

		Chunk * m_pools[2]; //two sets of pools, each garbage collection swaps between them, active is stored in m_cur_pool
		int     m_cur_pool; //which of the two pools are we using now
		Chunk * m_cur_chunk; //which chunk are we allocating from
		bool    m_in_gc;     //are we garbage collecting now?
		uint64_t m_alloced;  //how much memory are in all pools (active or not) combined
		uint64_t m_used;     //how much memory is used by data blocks, not including the header
		uint64_t m_data_blocks; //how many data allocations are in the current pool
		uint64_t m_next_gc;  //garbage collect when m_next_gc < m_used

		std::set<GCRoot*> m_roots;

		void collect();

		uchar *next(size_t size, uint16_t type)
		{
			assert(size < (CHUNK_SIZE >> 4));

			if(!m_in_gc)
				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Alloc %u type %x ... \n", (unsigned)size, type);

			size += (8 - size % 8) % 8; //8 byte align

			while(1){
				Data * data = m_cur_chunk->alloc(size + sizeof(Data));

				if(data){
					m_used += size;
					m_data_blocks++;

					data->m_type = type;
					data->m_forward = 0;
					data->m_pool = m_cur_pool;
					data->m_count = size;

					if(!m_in_gc)
						TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "alloc return %p\n", data->m_data);

					return data->m_data;
				}

				if(m_cur_chunk->m_next)
				{ //advance
					m_cur_chunk = m_cur_chunk->m_next;
					m_cur_chunk->init(m_cur_chunk->m_capacity);
				} else {
					//allocate a new chunk
					Chunk * c = new_chunk();

					m_cur_chunk->m_next = c;
					m_cur_chunk = c;
				}
			}
			return NULL;
		}

		Chunk * new_chunk()
		{
			Chunk * c = (Chunk *)malloc(CHUNK_SIZE);
			c->init(CHUNK_SIZE - sizeof(Chunk));
			DO_DEBUG VALGRIND_MAKE_MEM_NOACCESS(c->m_data, CHUNK_SIZE);
			m_alloced += CHUNK_SIZE;
			return c;
		}

	public:
		Semispace()
		 : m_cur_pool(0), m_in_gc(false), m_alloced(0), m_used(0), m_data_blocks(0), m_next_gc(1<<20)
		{
			m_pools[0] = new_chunk();
			m_pools[1] = NULL;

			m_cur_chunk = m_pools[m_cur_pool];
		}

		template <typename tObj>
		tObj *alloc(size_t extra, uint16_t type)
		{
			return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type));
		}

		template <typename tObj>
		bool validate(tObj * from)
		{
			if(m_in_gc)
				return true;
			else
				return (Data::ptr_for(from)->m_pool == m_cur_pool);
		}

		template <typename tObj>
		bool mark(tObj **from_ptr)
		{
			tObj *from = *from_ptr;
			Data * old = (Data*)(from) - 1;

			if(old->m_pool == m_cur_pool){
				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X already moved\n", from, old->m_type);
				return from;
			}

			if(old->m_forward){
				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X => %p\n", from, old->m_type, (*(tObj**)from));
				*from_ptr = *(tObj**)from;
				return true;
			}

			tObj * n = (tObj*)next(old->m_count, old->m_type);
			memcpy(n, from, old->m_count);

			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X <= %p\n", from, old->m_type, n);

			old->m_forward = 1;
			// put the new location in the old object's space
			*(tObj**)from = n;
			// change the marked pointer
			*from_ptr = n;
			return true;
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

