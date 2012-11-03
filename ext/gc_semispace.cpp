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
				// must not be forwarding pointers in the new heap.
				assert((d->m_forward) == 0);
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
			}

			c->m_used = 0;
		}

		//clear unused pinned objects
		std::vector<Data*> new_pinned_objs;
		for(std::vector<Data*>::iterator i = m_pinned_objs.begin(); i != m_pinned_objs.end(); ++i)
		{
			if((*i)->m_pool == m_cur_pool)
				new_pinned_objs.push_back(*i);
			else
				free(*i);
		}
		m_pinned_objs.swap(new_pinned_objs);

		//decide on the next gc cycle
		m_next_gc = std::max(CHUNK_SIZE*0.9, m_used * GC_GROWTH_LIMIT);

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Done GC, %"PRIu64" used in %"PRIu64" data blocks\n", m_used, m_data_blocks);

		m_in_gc = false;
	}
}

