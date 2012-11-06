#pragma once
#include "channel9.hpp"

namespace Channel9
{
	template <typename tPtr>
	bool in_nursery(const tPtr *ptr);
	bool in_nursery(const Value &val);

	// Implements an object nursery for young objects. Really simple allocator, pretty much just shoves
	// the data in and goes on its merry way. When it's time to collect it (by default when it has less
	// than 1/10 available space), it just moves its entire active set into the main object pool.
	// As an invariant, whenever an inner collection is taking place there should be nothing pointing
	// at the nursery as all objects (and references) should have been moved to the inner collector.
	template <typename tInnerGC>
	class GC::Nursery
	{
		struct Remembered
		{
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

		std::set<GCRoot*> m_roots;
		tInnerGC m_inner_gc;
		enum {
			STATE_NORMAL,
			STATE_NURSERY_COLLECT,
			STATE_INNER_COLLECT,
		} m_state;

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
			uint16_t m_type;
			uint16_t m_flags;
			uint8_t m_data[0];

			static const uint16_t FORWARD_FLAG = 0x1;

			inline Data *init(uint32_t size, uint16_t type){
				m_size = size;
				m_type = type;
				m_flags = 0;
				return this;
			}

			template <typename tPtr>
			static Data *from_ptr(const tPtr *ptr)
			{
				assert(Channel9::in_nursery(ptr));
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

	public:
		Nursery(size_t size = 1<<22)
		 : m_state(STATE_NORMAL),
		   m_size(size),
		   m_free(size),
		   m_min_free(1<<14),
		   m_data_blocks(0)
		{
			m_data = m_next_pos = (uint8_t*)malloc(size);
			m_remembered_end = m_remembered_set = (Remembered*)(m_data + size);
		}

		// extra is how much to allocate, type is one of Channel9::ValueType, return the new location
		// likely to call one of the more specific versions below
		template <typename tObj>
		tObj *alloc(size_t extra, uint16_t type, bool pinned = false)
		{
			size_t object_size = sizeof(tObj) + extra;
			size_t data_size   = sizeof(Data) + object_size;
			if (!pinned && data_size < m_free)
			{
				Data *data = (Data*)m_next_pos;
				m_next_pos += data_size;
				m_free -= data_size;
				m_data_blocks++;
				data->init(object_size, type);
				return (tObj*)data->m_data;
			} else {
				return m_inner_gc.alloc<tObj>(extra, type, pinned);
			}
		}

		//potentially faster versions to be called only if the size is known at compile time
		template <typename tObj>
		tObj *alloc_small(size_t extra, uint16_t type)
		{
			return alloc<tObj>(extra, type);
		}
		template <typename tObj>
		tObj *alloc_med(size_t extra, uint16_t type)
		{
			return m_inner_gc.alloc_med<tObj>(extra, type);
		}
		template <typename tObj>
		tObj *alloc_big(size_t extra, uint16_t type)
		{
			return m_inner_gc.alloc_big<tObj>(extra, type);
		}
		template <typename tObj>
		tObj *alloc_pinned(size_t extra, uint16_t type)
		{
			return m_inner_gc.alloc_pinned<tObj>(extra, type);
		}

		// notify the gc that an obj is pointed to, might mark it, might move it, might do something else. Returns true if it moved
		template <typename tObj>
		bool mark(tObj ** from);

		// is this object valid? only to be used for debugging
		template <typename tObj> bool validate(tObj * obj)
		{
			return true;
		}

		// read a pointer and do anything necessary to it.
		template <typename tRef>
		tRef read_ptr(const tRef &obj)
		{
			return m_inner_gc.read_ptr(obj);
		}

		// tell the GC that obj will contain a reference to the object pointed to by ptr
		template <typename tRef, typename tVal>
		void write_ptr(tRef &ref, const tVal &val)
		{
			assert(sizeof(val) == sizeof(uintptr_t));
			if (!Channel9::in_nursery(&ref) && Channel9::in_nursery(val))
			{
				// TODO: What to do when this happens? Maybe it should be a deque anyways.
				assert(m_free >= sizeof(Remembered));

				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Write barrier, adding: %p(%s) <- %p(%s)\n", &ref, Channel9::in_nursery(&ref)? "yes":"no", *(void**)&val, Channel9::in_nursery(val)? "yes":"no");

				m_free -= sizeof(Remembered);
				Remembered *r = --m_remembered_set;
				r->location = (uintptr_t*)&ref;
				r->val = *(uintptr_t*)&val;
			}
			m_inner_gc.write_ptr(ref, val);
		}

		template <typename tPtr>
		bool in_nursery(const tPtr *ptr)
		{
			return ((uint8_t*)ptr >= m_data && (uint8_t*)ptr < (uint8_t*)m_remembered_set);
		}

		template <typename tObj>
		void update_ptr(Data *data, tObj **ptr)
		{
			// we have to be careful in here because the pointer may have originally been a
			// value. If so, we need to do the right thing here.
			// TODO: If the location has been changed to an integer that just happens to correspond
			// to a pointer, how do we deal with that?
			uintptr_t *manip_ptr = (uintptr_t*)ptr;
			tObj *optr = (tObj*)(*manip_ptr & POINTER_MASK);
			tObj *nptr = m_inner_gc.alloc<tObj>(data->m_size - sizeof(tObj), data->m_type);
			memcpy(nptr, optr, data->m_size);
			data->set_forward(nptr);
			*manip_ptr = (*manip_ptr & ~POINTER_MASK) | ((uintptr_t)nptr & POINTER_MASK);
			TRACE_QUIET_PRINTF(TRACE_GC, TRACE_DEBUG, " moved to %p\n", nptr);
			gc_scan(nptr);
			TRACE_DO(TRACE_GC, TRACE_INFO){
				m_moved_bytes += data->m_size + sizeof(Data);
				m_moved_data++;
			}
		}
		void collect();

		void safe_point()
		{
			if (m_inner_gc.need_collect() || m_free < m_min_free)
			{
				m_state = STATE_NURSERY_COLLECT;
				collect();
			}
			m_state = STATE_INNER_COLLECT;
			m_inner_gc.safe_point();
			m_state = STATE_NORMAL;
		}

		void register_root(GCRoot *root)
		{
			m_inner_gc.register_root(root);
			m_roots.insert(root);
		}
		void unregister_root(GCRoot *root)
		{
			m_inner_gc.unregister_root(root);
			m_roots.erase(root);
		}
	};

	template <typename tInnerGC>
	template <typename tObj>
	bool GC::Nursery<tInnerGC>::mark(tObj ** from)
	{
		switch (m_state)
		{
		case STATE_NURSERY_COLLECT:
			if (in_nursery(*from))
			{
				Data *data = Data::from_ptr(*from);
				tObj *nptr = (tObj*)data->forward_addr();
				if (nptr)
				{
					*from = nptr;
				} else {
					nptr = m_inner_gc.alloc<tObj>(data->m_size - sizeof(tObj), data->m_type);
					memcpy(nptr, *from, data->m_size);
					data->set_forward(nptr);
					*from = nptr;
					gc_scan(nptr);
					TRACE_DO(TRACE_GC, TRACE_INFO){
						m_moved_bytes += data->m_size + sizeof(Data);
						m_moved_data++;
					}
				}
				return true;
			}
			// the nursery collector stops when it reaches an object not
			// in the nursery.
			return false;
		case STATE_INNER_COLLECT:
			return m_inner_gc.mark(from);
		case STATE_NORMAL:
			assert(false && "Marking object while not in a collection.");
			return false;
		}
		return false;
	}

	template <typename tInnerGC>
	void GC::Nursery<tInnerGC>::collect()
	{
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Nursery collection begun (%"PRIu64" objects, free: %"PRIu64"/%"PRIu64")\n", (uint64_t)m_data_blocks, (uint64_t)m_free, uint64_t(m_size));
		TRACE_DO(TRACE_GC, TRACE_INFO) m_moved_bytes = m_moved_data = 0;
		// first, scan the roots, which triggers a DFS through part the live set in the nursery
		for (std::set<GCRoot*>::iterator it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}

		// we only look through the external pointers in the remembered set,
		// not the full root set. This also triggers a partial dfs through the live set,
		// and updates the forwarding pointers
		Remembered *it = m_remembered_set;

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Updating %"PRIu64" pointers in remembered set\n", uint64_t(m_remembered_end - m_remembered_set));
		while (it != m_remembered_end)
		{
			uintptr_t raw = (*it->location & POINTER_MASK);
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Updating pointer %p (was set to %p, current value %p):", it->location, (void*)it->val, (void*)*it->location);
			if (it->val == *it->location)
			{
				Data *data = Data::from_ptr((uint8_t*)raw);
				uint8_t *nptr = data->forward_addr();
				if (nptr)
				{
					TRACE_QUIET_PRINTF(TRACE_GC, TRACE_DEBUG, " to %p (in forward table)\n", nptr);
					*it->location = (*it->location & ~POINTER_MASK) | ((uintptr_t)nptr & POINTER_MASK);
				} else {
					switch(data->m_type)
					{
					case STRING:  			update_ptr(data, (String**)  it->location); break;
					case TUPLE:   			update_ptr(data, (Tuple**)   it->location); break;
					case MESSAGE: 			update_ptr(data, (Message**) it->location); break;
					case CALLABLE_CONTEXT: 	update_ptr(data, (CallableContext**) it->location); break;
					case RUNNING_CONTEXT:	update_ptr(data, (RunningContext**)  it->location); break;
					case RUNNABLE_CONTEXT: 	update_ptr(data, (RunnableContext**) it->location); break;
					case VARIABLE_FRAME:   	update_ptr(data, (VariableFrame**)   it->location); break;
					default: assert(false && "Unknown GC type");
					}
				}
			} else {
				TRACE_QUIET_PRINTF(TRACE_GC, TRACE_DEBUG, " did nothing.\n");
			}
			it++;
		}

		// and then reset the nursery space
		m_next_pos = m_data;
		m_remembered_set = m_remembered_end = (Remembered*)(m_data + m_size);
		m_free = m_size;
		m_data_blocks = 0;

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Nursery collection done, moved %"PRIu64" Data/%"PRIu64" bytes to the inner collector\n", uint64_t(m_moved_data), uint64_t(m_moved_bytes));
	}

	template <typename tPtr>
	bool in_nursery(const tPtr *ptr)
	{
		return value_pool.in_nursery(ptr);
	}
}

