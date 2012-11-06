#include "channel9.hpp"
#include "memory_pool.hpp"

#include "value.hpp"

#include "string.hpp"
#include "tuple.hpp"
#include "message.hpp"
#include "context.hpp"
#include "variable_frame.hpp"

namespace Channel9
{

	void GC::Semispace::register_root(GCRoot *root)
	{
		m_roots.insert(root);
	}
	void GC::Semispace::unregister_root(GCRoot *root)
	{
		m_roots.erase(root);
	}

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
	void GC::Semispace::scan(Data * d)
	{
		// must not be forwarding pointers in the new heap.
		assert((d->forward()) == 0);
		TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan Obj %p, type %X\n", d->m_data, d->m_type);
		switch(d->m_type)
		{
		case STRING:  			gc_scan( (String*)  (d->m_data)); break;
		case TUPLE:   			gc_scan( (Tuple*)   (d->m_data)); break;
		case MESSAGE: 			gc_scan( (Message*) (d->m_data)); break;
		case CALLABLE_CONTEXT: 	gc_scan( (CallableContext*) (d->m_data)); break;
		case RUNNING_CONTEXT:	gc_scan( (RunningContext*)  (d->m_data)); break;
		case RUNNABLE_CONTEXT: 	gc_scan( (RunnableContext*) (d->m_data)); break;
		case VARIABLE_FRAME:   	gc_scan( (VariableFrame*)   (d->m_data)); break;
		default: assert(false && "Unknown GC type");
		}
	}
	void GC::Semispace::collect()
	{
		m_in_gc = true;

		//switch pools
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Start GC, old pool %p, new pool %p, %"PRIu64" used in %"PRIu64" data blocks\n", m_pools[m_cur_pool], m_pools[!m_cur_pool], m_used, m_data_blocks);

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
		m_next_gc = std::max(CHUNK_SIZE*0.9, m_used * GC_GROWTH_LIMIT);

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

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Done GC, %"PRIu64" used in %"PRIu64" data blocks, next collection at %"PRIu64"\n", m_used, m_data_blocks, m_next_gc);

		m_in_gc = false;
	}
}

