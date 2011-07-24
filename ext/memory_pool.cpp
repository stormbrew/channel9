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
	GCRoot::GCRoot(MemoryPool &pool)
	 : m_pool(pool)
	{
		m_pool.register_root(this);
	}

	GCRoot::~GCRoot()
	{
		m_pool.unregister_root(this);
	}

	void MemoryPool::register_root(GCRoot *root)
	{
		m_roots.insert(root);
	}
	void MemoryPool::unregister_root(GCRoot *root)
	{
		m_roots.erase(root);
	}

	void MemoryPool::collect()
	{
		m_in_gc = true;

		//switch pools
		DO_TRACEGC printf("Start GC, old pool %p, new pool %p, %llu used in %llu data blocks\n", m_pools[m_cur_pool], m_pools[!m_cur_pool], m_used, m_data_blocks);

		m_cur_pool = !m_cur_pool;
		m_cur_chunk = m_pools[m_cur_pool];
		m_used = 0;
		m_data_blocks = 0;

		//clear this pool
		for(Chunk * c = m_cur_chunk; c; c = c->m_next)
		{
			c->m_used = 0;
		}

		DO_TRACEGC printf("Scan roots\n");

		//scan the roots
		std::set<GCRoot*>::iterator it;
		for (it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}

		//scan the new heap copying in the reachable set
		for(Chunk * c = m_pools[m_cur_pool]; c; c = c->m_next)
		{
			DO_TRACEGC printf("Scan Chunk %p\n", c);
			for(Data * d = c->begin(); d != c->end(); d = d->next())
			{
				DO_TRACEGC printf("Scan Obj %p, type %X\n", d->m_data, d->m_type);
				switch(d->m_type)
				{
				case GC_STRING:  gc_scan( (String*)  (d->m_data)); break;
				case GC_TUPLE:   gc_scan( (Tuple*)   (d->m_data)); break;
				case GC_MESSAGE: gc_scan( (Message*) (d->m_data)); break;
				case GC_CALLABLE_CONTEXT: gc_scan( (CallableContext*) (d->m_data)); break;
				case GC_RUNNABLE_CONTEXT: gc_scan( (RunnableContext*) (d->m_data)); break;
				case GC_VARIABLE_FRAME:   gc_scan( (VariableFrame*)   (d->m_data)); break;
				case GC_FORWARD: assert(d->m_type != GC_FORWARD); // newly built heap, can't be forwards.
				default: assert(false && "Unknown GC type");
				}
			}
		}

		if(m_cur_chunk->m_next == NULL) //TODO: better heuristic for when to allocate a new chunk
		{//still on the last chunk, must be fairly full, allocate an extra chunk
			int new_size = m_cur_chunk->m_capacity * GROWTH;

			Chunk * c = new_chunk(new_size);
			m_cur_chunk->m_next = c;

			DO_TRACEGC printf("Alloc new trailing chunk\n");
		}

		for(Chunk * c = m_pools[!m_cur_pool]; c; c = c->m_next)
		{
			DO_DEBUG c->deadbeef();
			c->m_used = 0;
		}



		DO_TRACEGC printf("Done GC, %llu used in %llu data blocks\n", m_used, m_data_blocks);

		m_in_gc = false;
	}
}

