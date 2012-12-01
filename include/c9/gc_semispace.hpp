#pragma once

#include <set>
#include <vector>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

#include "c9/channel9.hpp"
#include "c9/bittwiddle.hpp"

#include "memcheck.h"

namespace Channel9
{
	class GC::Semispace : public GC
	{
	public:
		struct Data
		{
			uint32_t m_count; //number of bytes of memory in this allocation
			uint8_t m_generation;
			uint8_t m_type;
			uint16_t m_flags;

			enum {
				PINNED = 0x1000,
				FORWARD = 0x2000,
				POOL_MASK = 0x0001,
			};
			uint8_t  m_data[0]; //the actual data, 8 byte aligned

			inline Data *init(uint32_t count, uint8_t type, bool pool, bool pin){
				m_count = count;
				m_generation = GEN_TENURE;
				m_type = type;
				m_flags = (uint16_t)pool | ((uint16_t)pin << 12);
				return this;
			}
			bool pool() { return m_flags & POOL_MASK; }
			bool pinned() { return m_flags & PINNED; }
			bool forward() { return m_flags & FORWARD; }
			void set_forward() { m_flags |= FORWARD; }
			void set_pool(bool pool) { m_flags = pool | ((m_flags ^ POOL_MASK) & m_flags); }

			Data *next() const { return (Data*)((uint8_t*)(this + 1) + m_count); }
			static Data *ptr_for(const void *ptr) { return (Data*)ptr - 1; }
		};

	private:

		struct Chunk
		{
			Chunk *  m_next; //linked list of chunks
			uint32_t m_capacity; //in bytes
			uint32_t m_used;     //in bytes
			uint8_t  m_data[0];  //actual memory

			void init(uint32_t capacity)
			{
				m_next = NULL;
				m_capacity = capacity;
				m_used = 0;
				VALGRIND_CREATE_MEMPOOL(m_data, 0, false);
			}

			bool has_space(size_t size)
			{
				return m_used + size <= m_capacity;
			}
			inline Data * alloc(size_t size)
			{
				assert(has_space(size));
				Data * ret = (Data*)(m_data + m_used);
				m_used += size;
				VALGRIND_MEMPOOL_ALLOC(m_data, ret, size);
				return ret;
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

		static const uint64_t   GC_GROWTH_LIMIT = 2;
		static const uint64_t CHUNK_SIZE = (2<<20) - sizeof(Chunk) - 8; // 2mb (-8 for the malloc header)

		Chunk * m_pools[2]; //two sets of pools, each garbage collection swaps between them, active is stored in m_cur_pool
		int     m_cur_pool; //which of the two pools are we using now
		Chunk * m_cur_chunk; //which chunk are we allocating from
		bool    m_in_gc;     //are we garbage collecting now?
		uint64_t m_alloced;  //how much memory are in all pools (active or not) combined
		uint64_t m_used;     //how much memory is used by data blocks, not including the header
		uint64_t m_data_blocks; //how many data allocations are in the current pool
		uint64_t m_next_gc;  //garbage collect when m_next_gc < m_used

		std::vector<Data *> m_pinned_objs;

		void scan(Data * d)
		{
			assert((d->forward()) == 0);
			GC::scan(d->m_data, ValueType(d->m_type));
		}

		uint8_t *next_slow(size_t size, size_t alloc_size, uint16_t type);
		inline uint8_t *next(size_t size, uint16_t type)// __attribute__((always_inline))
		{
			size = ceil_power2(size, 3); //8 byte align
			size_t alloc_size = size + sizeof(Data);

			m_used += alloc_size;
			m_data_blocks++;

			if (m_cur_chunk->has_space(alloc_size))
			{
				Data * data = m_cur_chunk->alloc(alloc_size)->init(size, type, m_cur_pool, false);

				if(!m_in_gc)
					TRACE_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "fast alloc %u type %x return %p\n", (unsigned)size, type, data->m_data);

				return data->m_data;
			} else {
				return next_slow(size, alloc_size, type);
			}
		}

		Chunk * new_chunk(size_t alloc_size = 0)
		{
			Chunk *c;
			if(alloc_size < CHUNK_SIZE)
				alloc_size = CHUNK_SIZE;

			// try to steal a chunk from the other list first
			if (!m_in_gc && (c = m_pools[!m_cur_pool]))
			{
				// if the first one is big enough, use it.
				if (c->m_capacity >= alloc_size)
				{
					m_pools[!m_cur_pool] = c->m_next;
					c->m_next = NULL;
					return c;
				} else {
					// otherwise try to find one that is.
					Chunk *prev = c;
					while (prev = c, c = c->m_next)
					{
						if (c->m_capacity >= alloc_size)
						{
							prev->m_next = c->m_next;
							c->m_next = NULL;
							return c;
						}
					}
				}
			}

			c = (Chunk *)malloc(alloc_size + sizeof(Chunk));
			c->init(alloc_size);
#if !defined(NVALGRIND) // this shouldn't be necessary, but it causes a warning otherwise.
			VALGRIND_MAKE_MEM_NOACCESS(c->m_data, alloc_size);
#endif
			m_alloced += alloc_size;
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

		template <typename tObj> tObj *alloc(size_t extra, uint16_t type, bool pinned = false)
		{
			if(pinned)
				return alloc_pinned<tObj>(extra, type);
			else
				return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type));
		}

		template <typename tObj> tObj *alloc_small(size_t extra, uint16_t type){ return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type)); }
		template <typename tObj> tObj *alloc_med  (size_t extra, uint16_t type){ return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type)); }
		template <typename tObj> tObj *alloc_big  (size_t extra, uint16_t type){ return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type)); }

		template <typename tObj> tObj *alloc_pinned(size_t extra, uint16_t type){
			size_t size = sizeof(tObj) + extra;
			size = ceil_power2(size, 3); //8 byte align

			Data * data = (Data*) malloc(size + sizeof(Data));
			data->init(size, type, m_cur_pool, true);
			m_pinned_objs.push_back(data);

			m_used += size;
			m_data_blocks++;

			return reinterpret_cast<tObj*>(data->m_data);
		}

		void *raw_alloc(size_t size, ValueType type, bool pinned)
		{
			if(pinned)
				return alloc_pinned<char>(size, (uint16_t)type);
			else
				return next(size, (uint16_t)type);
		}
		// already allocated object to the generation in question.
		inline void *promote(void *obj, size_t size, ValueType type, bool pinned)
		{
			void *n = raw_alloc(size, type, pinned);
			memcpy(n, obj, size);
			return n;
		}

		template <typename tObj>
		bool validate(tObj * from)
		{
			if(m_in_gc)
				return true;
			else
				return (Data::ptr_for(from)->pool() == m_cur_pool);
		}

		bool mark(void *obj, uintptr_t *from_ptr);

		void scan(void *p)
		{
			scan(Data::ptr_for(p));
		}

		// make sure this object is ready to be read from
		template <typename tObj>
		void read_ptr(tObj * obj) { }

		template <typename tField, typename tVal>
		void write_ptr(void *obj, tField &field, const tVal &val)
		{
		}

		void collect();
		bool need_collect()
		{
			return m_next_gc < m_used;
		}
	};
}

