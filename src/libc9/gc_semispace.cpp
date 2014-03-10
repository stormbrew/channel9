#include "c9/channel9.hpp"
#include "c9/gc.hpp"

#include "c9/value.hpp"

#include "c9/string.hpp"
#include "c9/tuple.hpp"
#include "c9/message.hpp"
#include "c9/context.hpp"
#include "c9/variable_frame.hpp"

namespace Channel9
{
	uint8_t *GC::Semispace::next_slow(size_t size, size_t alloc_size, uint16_t type)
	{
		Chunk * chunk = m_cur_chunk;

		while(1){
			if (chunk->has_space(alloc_size))
			{
				Data * data = chunk->alloc(alloc_size)->init(size, type, m_cur_pool, false);

				if(!m_in_gc)
					TRACE_PRINTF(TRACE_ALLOC, TRACE_DEBUG, "slow alloc %u type %x return %p\n", (unsigned)size, type, data->m_data);

				return data->m_data;
			}

			if(chunk->m_next)
			{ //advance to allocate out of the next chunk
				chunk = chunk->m_next;
			} else {
				//allocate a new chunk
				Chunk * c = new_chunk(alloc_size);

				chunk->m_next = c;
				chunk = c;
			}

			// only advance if there wasn't enough room for a small object, otherwise put medium and big objects
			// in the next chunk while continuing to put small ones in the current chunk
			if(alloc_size < SMALL)
				m_cur_chunk = chunk;
		}
	}
	void GC::Semispace::collect()
	{
		m_in_gc = true;

		//switch pools
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Start GC, old pool %p, new pool %p, %" PRIu64 " used in %" PRIu64 " data blocks\n", m_pools[m_cur_pool], m_pools[!m_cur_pool], m_used, m_data_blocks);

		m_cur_pool = !m_cur_pool;

		if(m_pools[m_cur_pool] == NULL)
			m_pools[m_cur_pool] = new_chunk();

		m_cur_chunk = m_pools[m_cur_pool];
		m_used = 0;
		m_data_blocks = 0;

		TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan roots\n");

		//scan the roots
		std::set<GCRoot*>::iterator it;
		for (it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}

		//scan the new heap copying in the reachable set
		for(Chunk * c = m_pools[m_cur_pool]; c; c = c->m_next)
		{
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan Chunk %p\n", c);
			for(Data * d = c->begin(); d != c->end(); d = d->next())
			{
				scan(d);
			}
		}

		//free the old pool to encourage bad code to segfault
		DO_DEBUG {
			Chunk * c = m_pools[!m_cur_pool];
			while(c){
				Chunk * f = c;
				VALGRIND_DESTROY_MEMPOOL(c->m_data);
				c = c->m_next;
				free(f);
			}
			m_pools[!m_cur_pool] = NULL;
		}

		//clear the old pool
		for(Chunk * c = m_pools[!m_cur_pool]; c; c = c->m_next)
		{
			DO_DEBUG {
				c->deadbeef();
				VALGRIND_DESTROY_MEMPOOL(c->m_data);
			} else {
				VALGRIND_MEMPOOL_TRIM(c->m_data, 0, 0);
			}

			c->m_used = 0;
		}

		//clear unused pinned objects
		std::vector<Data*> new_pinned_objs;
		for(std::vector<Data*>::iterator i = m_pinned_objs.begin(); i != m_pinned_objs.end(); ++i)
		{
			if((*i)->pool() == m_cur_pool)
				new_pinned_objs.push_back(*i);
			else
				free(*i);
		}
		m_pinned_objs.swap(new_pinned_objs);

		//decide on the next gc cycle
		m_next_gc = std::max(CHUNK_SIZE*0.9, double(m_used) * GC_GROWTH_LIMIT);

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Sweeping CallableContext objects\n");

		CallableContext::sweep();

		TRACE_OUT(TRACE_GC, TRACE_INFO) {
			tprintf("Area stats:\n");
			unsigned long area_size = 0;
			unsigned long area_used = 0;
			int count = 0;
			for (Chunk *c = m_pools[!m_cur_pool]; c; c = c->m_next)
			{
				count++;
				area_size += c->m_capacity;
			}
			tprintf("Old pool: 0/%lu in %i\n", area_size, count);
			area_size = 0;
			count = 0;
			for (Chunk *c = m_pools[m_cur_pool]; c; c = c->m_next)
			{
				area_size += c->m_capacity;
				area_used += c->m_used;
				count++;
			}
			tprintf("New pool: %lu/%lu in %i\n", area_used, area_size, count);
			area_used = 0;
			count = 0;
			std::vector<Data*> new_pinned_objs;
			for(std::vector<Data*>::iterator i = m_pinned_objs.begin(); i != m_pinned_objs.end(); ++i)
			{
				area_used += (*i)->m_count;
				count++;
			}
			tprintf("Pinned: %lu in %i\n", area_used, count);
		}

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Done GC, %" PRIu64 " used in %" PRIu64 " data blocks, next collection at %" PRIu64 "\n", m_used, m_data_blocks, m_next_gc);

		m_in_gc = false;
	}

	bool GC::Semispace::mark(void *obj, uintptr_t *from_ptr)
	{
		void *from = raw_tagged_ptr(*from_ptr);
		Data * old = Data::ptr_for(from);

		// we should never be marking an object that's in the nursery here.
		assert(is_tenure(from));

		if(old->pool() == m_cur_pool){
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X already moved\n", from, old->m_type);
			return false;
		}

		if(old->pinned()){
			old->set_pool(m_cur_pool);
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X pinned, update pool, recursing\n", from, old->m_type);
			GC::scan(from, (ValueType)old->m_type);
			return false;
		}

		if(old->forward()){
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X => %p\n", from, old->m_type, (*(void**)from));
			update_tagged_ptr(from_ptr, *(void**)from);
			return true;
		}

		void * n = (void*)next(old->m_count, old->m_type);
		memcpy(n, from, old->m_count);

		TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Move %p, type %X <= %p\n", from, old->m_type, n);

		old->set_forward();
		// put the new location in the old object's space
		*(void**)from = n;
		// change the marked pointer
		update_tagged_ptr(from_ptr, n);
		return true;
	}
}

