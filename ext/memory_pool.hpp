#pragma once

#include <set>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

namespace Channel9
{
	class MemoryPool;

	// Base class for GC root objects. Should inherit privately.
	class GCRoot
	{
	private:
		MemoryPool &m_pool;

	protected:
		GCRoot(MemoryPool &pool);

		MemoryPool &pool() const { return m_pool; }

	public:
		virtual void scan() = 0;

		virtual ~GCRoot();
	};

	class MemoryPool
	{
	public:
		enum
		{
			GC_FORWARD          = 0xF, //already been moved

			GC_STRING           = 0x3,
			GC_TUPLE            = 0x4,
			GC_MESSAGE          = 0x5,
			GC_CALLABLE_CONTEXT = 0x6,
			GC_RUNNABLE_CONTEXT = 0x7,
			GC_VARIABLE_FRAME   = 0x8,
		};

	private:

		static const float GROWTH = 1.2;


		typedef unsigned char uchar;

		struct Data
		{
			uint16_t m_type;
			uint16_t m_pool;
			uint32_t m_count; //number of bytes of memory in this allocation
			uchar    m_data[0]; //the actual data, 8 byte aligned

			Data *next() const { return (Data*)((uchar*)(this + 1) + m_count); }
			static Data *ptr_for(const void *ptr) { return (Data*)ptr - 1; }
		};

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
			}

			Data * alloc(size_t size)
			{
				if(m_used + size <= m_capacity)
				{
					Data * ret = (Data*)(m_data + m_used);
					m_used += size;
					return ret;
				}
				return NULL;
			}
			Data * begin(){ return (Data *) m_data; }
			Data * end()  { return (Data *) (m_data + m_used); }
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
		bool    m_in_gc;     //are we garbage collecting now? if so, just allocate a new chunk if the last one is full
		size_t  m_initial_size; //how big is the first chunk
		uint64_t m_alloced;  //how much memory are in all pools (active or not) combined
		uint64_t m_used;     //how much memory is used by data blocks, not including the header
		uint64_t m_data_blocks; //how many data allocations are in the current pool

		std::set<GCRoot*> m_roots;

		void collect();

		uchar *next(size_t size, uint32_t type)
		{
			bool collected = false;
			assert(size < 10000);

			if(!m_in_gc)
				DO_TRACEGC printf("Alloc %u type %x ... ", (unsigned)size, type);

			assert(type != GC_FORWARD); // never alloc a forwarding ref.
			size += (8 - size % 8) % 8; //8 byte align

			while(1){
				Data * data = m_cur_chunk->alloc(size + sizeof(Data));

				if(data){
					m_used += size;
					m_data_blocks++;

					data->m_type = type;
					data->m_pool = m_cur_pool;
					data->m_count = size;

					if(!m_in_gc)
						DO_TRACEGC printf("alloc return %p\n", data->m_data);

					return data->m_data;
				}

				if(m_cur_chunk->m_next)
				{ //advance
					m_cur_chunk = m_cur_chunk->m_next;
					m_cur_chunk->init(m_cur_chunk->m_capacity);
				} else if(m_in_gc || collected) {
					//allocate a new chunk
					size_t new_size = m_cur_chunk->m_capacity * GROWTH;

					Chunk * c = new_chunk(new_size);

					m_cur_chunk->m_next = c;
					m_cur_chunk = c;
				} else {
					collect();
					collected = true;
				}
			}
			return NULL;
		}

		Chunk * new_chunk(size_t size)
		{
			size += (8 - size % 8) % 8; //8 byte align
			Chunk * c = (Chunk *)malloc(sizeof(Chunk) + size);
			c->init(size);
			m_alloced += size;
			return c;
		}

	public:
		MemoryPool(size_t initial_size)
		 : m_cur_pool(0), m_in_gc(false), m_initial_size(initial_size), m_alloced(0), m_used(0), m_data_blocks(0)
		{
			m_pools[0] = new_chunk(m_initial_size);
			m_pools[1] = NULL;

			m_cur_chunk = m_pools[m_cur_pool];
		}

		template <typename tObj>
		tObj *alloc(size_t extra, uint32_t type)
		{
			return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type));
		}

		template <typename tObj>
		void validate(tObj * from)
		{
			if(!m_in_gc)
			{
				assert(Data::ptr_for(from)->m_pool == m_cur_pool);
			}
		}

		template <typename tObj>
		tObj *move(tObj * from)
		{
			Data * old = (Data*)(from) - 1;

			if(old->m_pool == m_cur_pool){
				DO_TRACEGC printf("Move %p, type %X already moved\n", from, old->m_type);
				return from;
			}

			if(old->m_type == GC_FORWARD){
				DO_TRACEGC printf("Move %p, type %X => %p\n", from, old->m_type, (*(tObj**)from));
				return *(tObj**)from;
			}

			tObj * n = (tObj*)next(old->m_count, old->m_type);
			memcpy(n, from, old->m_count);

			DO_TRACEGC printf("Move %p, type %X <= %p\n", from, old->m_type, n);

			old->m_type = GC_FORWARD;
			*(tObj**)from = n;
			return n;
		}

		void unregister_root(GCRoot *root);
		void register_root(GCRoot *root);
	};

	extern MemoryPool value_pool;
}

