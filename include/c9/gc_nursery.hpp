#pragma once
#include "c9/channel9.hpp"
#include "memcheck.h"

#include <stdexcept>

namespace Channel9
{
	// Implements an object nursery for young objects. Really simple allocator, pretty much just shoves
	// the data in and goes on its merry way. When it's time to collect it (by default when it has less
	// than 1/10 available space), it just moves its entire active set into the main object pool.
	// As an invariant, whenever an inner collection is taking place there should be nothing pointing
	// at the nursery as all objects (and references) should have been moved to the inner collector.
	class GC::Nursery : public GC
	{
		struct Remembered
		{
			void *in_object;
			uintptr_t *location;
			uintptr_t val;
		};

		// m_data is a pointer to the base address of the nursery chunk,
		// m_next_pos is the next position to allocate from,
		// m_remembered_set is the first item in the remembered set (also the end of the allocation)
		// m_remembered_end is the end of the remembered set and the end of the nursery chunk as a whole.
		// After a nursery collection, the nursery will be evacuated and m_next_pos will == m_data and m_end_pos will == m_data_end
		uint8_t *m_data;
		uint8_t *m_next_pos;
		Remembered *m_remembered_set;
		Remembered *m_remembered_end;

		size_t m_size;        // size of the whole space
		size_t m_free;        // currently empty space between the allocations and the remembered set
		size_t m_min_free;    // start a compaction once free < min_free
		size_t m_data_blocks; // number of data blocks that are allocated

		size_t m_moved_bytes;
		size_t m_moved_data;

		const static uint64_t POINTER_MASK = 0x00007fffffffffffULL;

		struct Data
		{
			uint32_t m_size;
			uint8_t m_generation;
			uint8_t m_type;
			uint16_t m_flags;
			uint8_t m_data[0];

			static const uint16_t FORWARD_FLAG = 0x1;

			inline Data *init(uint32_t size, uint8_t type, uint8_t generation){
				m_size = size;
				m_generation = generation;
				m_type = type;
				m_flags = 0;
				return this;
			}

			template <typename tPtr>
			static Data *from_ptr(const tPtr *ptr)
			{
				return (Data*)ptr - 1;
			}

			template <typename tObj>
			void set_forward(tObj *to)
			{
				m_flags |= FORWARD_FLAG;
				*(tObj**)m_data = to;
			}
			uint8_t *forward_addr() const
			{
				if (m_flags & FORWARD_FLAG)
					return *(uint8_t**)m_data;
				else
					return NULL;
			}
		};
		std::stack<void*>  m_scan_list;  // list of live objects left to scan

		uint8_t m_generation;
		GC *m_next;

	public:
		Nursery(size_t size, unsigned gen, GC *next)
		 : m_size(size),
		   m_free(size),
		   m_min_free(size/4),
		   m_data_blocks(0),
		   m_generation(gen),
		   m_next(next)
		{
			m_data = m_next_pos = (uint8_t*)malloc(size);
			VALGRIND_CREATE_MEMPOOL(m_data, 0, false)
			m_remembered_end = m_remembered_set = (Remembered*)(m_data + size);
		}

		inline void *raw_alloc(size_t object_size, ValueType type, bool pinned)
		{
			size_t data_size   = sizeof(Data) + object_size;
			if (!pinned && data_size < m_free)
			{
				Data *data = (Data*)m_next_pos;
				VALGRIND_MEMPOOL_ALLOC(m_data, m_next_pos, data_size);
				m_next_pos += data_size;
				m_free -= data_size;
				m_data_blocks++;
				data->init(object_size, type, m_generation);
				TRACE_PRINTF(TRACE_ALLOC, TRACE_SPAM,
					"Nursery%u Alloc %u type %x ... got %p ... alloc return %p\n",
					m_generation, (unsigned)object_size, type, data, data->m_data);
				return data->m_data;
			} else {
				return m_next->raw_alloc(object_size, type, pinned);
			}
		}
		// called by the next generation down to promote an
		// already allocated object to the generation in question.
		inline void *promote(void *obj, size_t size, ValueType type, bool pinned)
		{
			void *n = raw_alloc(size, type, pinned);
			memcpy(n, obj, size);
			return n;
		}

		// notify the gc that an obj is pointed to, might mark it, might move it, might do something else. Returns true if it moved
		bool mark(void *obj, uintptr_t *from);

		// is this object valid? only to be used for debugging
		template <typename tObj> bool validate(tObj * obj)
		{
			return true;
		}

		// read a pointer and do anything necessary to it.
		template <typename tRef>
		tRef read_ptr(const tRef &obj)
		{
		}

		// tell the GC that obj will contain a reference to the object pointed to by ptr
		template <typename tField, typename tVal>
		void write_ptr(void *obj, tField &field, const tVal &val)
		{
			assert(sizeof(val) == sizeof(uintptr_t));
			if ((!obj || generation_of(obj) > m_generation) && is_generation(val, m_generation))
			{
				// TODO: What to do when this happens? Maybe it should be a deque anyways.
				if (m_free < sizeof(Remembered))
					throw std::runtime_error("No free space for remembered set! PANIC!");

				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Write barrier (%u), adding: %p(%s) <- %p(%s)\n", m_generation, &field, obj && is_generation(&obj, m_generation)? "yes":"no", *(void**)&val, is_generation(val, m_generation)? "yes":"no");

				m_free -= sizeof(Remembered);
				Remembered *r = --m_remembered_set;
				VALGRIND_MEMPOOL_ALLOC(m_data, r, sizeof(Remembered));
				r->in_object = obj;
				r->location = (uintptr_t*)&field;
				r->val = *(uintptr_t*)&val;
			}
		}

		void collect();

		inline bool need_collect()
		{
			return m_free < m_min_free;
		}
	};
}
