#include "c9/channel9.hpp"
#include "c9/gc.hpp"
#include "c9/gc_nursery.hpp"

namespace Channel9
{
	bool GC::Nursery::mark(void *obj, uintptr_t *from_ptr)
	{
		void *from = raw_tagged_ptr(*from_ptr);
		switch (m_state)
		{
		case STATE_NURSERY_COLLECT:
			if (is_nursery(from))
			{
				Data *data = Data::from_ptr(from);
				void *nptr = (void*)data->forward_addr();
				if (nptr)
				{
					update_tagged_ptr(from_ptr, nptr);
					TRACE_PRINTF(TRACE_GC, TRACE_SPAM, "Nursery pointer fix at %p: %p -> %p\n", from_ptr, from, nptr);
				} else {
					// note that we get an extra byte because we're kind of circumventing
					// the interface to get an arbitrary block of bytes.
					// TODO: Make that less messy.
					nptr = normal_pool.alloc<char>(data->m_size, data->m_type);
					memcpy(nptr, from, data->m_size);
					data->set_forward(nptr);
					TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Nursery commit at %p: %p -> %p (%u bytes) in inner collector\n", from_ptr, from, nptr, data->m_size);

					update_tagged_ptr(from_ptr, nptr);
					m_scan_list.push(nptr);
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
			return normal_pool.mark(obj, from_ptr);
		case STATE_NORMAL:
			assert(false && "Marking object while not in a collection.");
			return false;
		}
		return false;
	}

	void GC::Nursery::collect()
	{
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Nursery collection begun (%"PRIu64" objects, free: %"PRIu64"/%"PRIu64")\n", (uint64_t)m_data_blocks, (uint64_t)m_free, uint64_t(m_size));
		TRACE_DO(TRACE_GC, TRACE_INFO) m_moved_bytes = m_moved_data = 0;

		// first, scan the roots, which triggers a DFS through part the live set in the nursery
		for (std::set<GCRoot*>::iterator it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}

		while(!m_scan_list.empty()) {
			void *p = m_scan_list.top();
			m_scan_list.pop();
			scan(p);
		}

		// we only look through the external pointers in the remembered set,
		// not the full root set. This also triggers a partial dfs through the live set,
		// and updates the forwarding pointers
		Remembered *it = m_remembered_set;

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Updating %"PRIu64" pointers in remembered set\n", uint64_t(m_remembered_end - m_remembered_set));
		while (it != m_remembered_end)
		{
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Updating pointer %p (was set to %p, current value %p):", it->location, (void*)it->val, (void*)*it->location);
			if (it->val == *it->location)
			{
				Data *data = Data::from_ptr((uint8_t*)raw_tagged_ptr(*it->location));
				uint8_t *nptr = data->forward_addr();
				if (nptr)
				{
					TRACE_QUIET_PRINTF(TRACE_GC, TRACE_DEBUG, " to %p (in forward table)\n", nptr);
					update_tagged_ptr(it->location, nptr);
				} else {
					mark(it->in_object, it->location);
				}
			} else {
				TRACE_QUIET_PRINTF(TRACE_GC, TRACE_DEBUG, " did nothing.\n");
			}
			it++;
		}

		while(!m_scan_list.empty()) {
			void *p = m_scan_list.top();
			m_scan_list.pop();
			scan(p);
		}

		// and then reset the nursery space
		m_next_pos = m_data;
		m_remembered_set = m_remembered_end = (Remembered*)(m_data + m_size);
		m_free = m_size;
		m_data_blocks = 0;
		VALGRIND_DESTROY_MEMPOOL(m_data);
		VALGRIND_CREATE_MEMPOOL(m_data, 0, false);

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Nursery collection done, moved %"PRIu64" Data/%"PRIu64" bytes to the inner collector\n", uint64_t(m_moved_data), uint64_t(m_moved_bytes));
	}
}
