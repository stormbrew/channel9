#pragma once

#include <set>
#include <stdlib.h>
#include <stdint.h>

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
			GC_FORWARD          = 0x0, //already been moved

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
			uint32_t m_type;
			uint32_t m_count; //number of bytes of memory in this allocation
			uchar    m_data[0]; //the actual data, 8 byte aligned
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

			uchar * alloc(size_t size)
			{
				if(m_used + size <= m_capacity)
				{
					uchar * ret = m_data + m_used;
					m_used += size;
					return ret;
				}
				return NULL;
			}
		};

		Chunk * m_pools[2]; //two sets of pools, each garbage collection swaps between them, active is stored in m_cur_pool
		int     m_cur_pool; //which of the two pools are we using now
		Chunk * m_cur_chunk; //which chunk are we allocating from
		bool    m_in_gc;     //are we garbage collecting now? if so, just allocate a new chunk if the last one is full

		std::set<GCRoot*> m_roots;

		void collect();

		uchar *next(size_t size, uint32_t type)
		{
			size += (8 - size % 8) % 8; //8 byte align

			while(1){
				uchar * ptr = m_cur_chunk->alloc(size + sizeof(Data));

				if(ptr){
					Data * data = (Data *)ptr;
					data->m_type = type;
					data->m_count = size;
					return (uchar *)(data->m_data);
				}

				if(m_cur_chunk->m_next)
				{ //advance
					m_cur_chunk = m_cur_chunk->m_next;
				} else {
					if(m_in_gc)
					{ //allocate a new chunk
						int new_size = m_cur_chunk->m_capacity * GROWTH;

						Chunk * c = (Chunk *)malloc(sizeof(Chunk) + new_size);
						c->init(new_size);

						m_cur_chunk->m_next = c;
						m_cur_chunk = c;
					} else {
						collect();
					}
				}
			}
			return NULL;
		}


	public:
		MemoryPool(size_t initial_size)
		 : m_cur_pool(0), m_in_gc(false)
		{
			m_pools[0] = (Chunk *)malloc(sizeof(Chunk) + initial_size);
			m_pools[0]->init(initial_size);

			m_pools[1] = (Chunk *)malloc(sizeof(Chunk) + initial_size);
			m_pools[1]->init(initial_size);

			m_cur_chunk = m_pools[m_cur_pool];
		}

		template <typename tObj>
		tObj *alloc(size_t extra, uint32_t type)
		{
			return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra, type));
		}


		void unregister_root(GCRoot *root);
		void register_root(GCRoot *root);
	};

	extern MemoryPool value_pool;
}

