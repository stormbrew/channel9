#include "channel9.hpp"
#include "memory_pool.hpp"

#include "value.hpp"

#include "string.hpp"
#include "tuple.hpp"
#include "message.hpp"
#include "context.hpp"
#include "variable_frame.hpp"

#include "gc_markcompact.hpp"

namespace Channel9
{

	void GC::Markcompact::register_root(GCRoot *root)
	{
		m_roots.insert(root);
	}
	void GC::Markcompact::unregister_root(GCRoot *root)
	{
		m_roots.erase(root);
	}

	void GC::Markcompact::collect()
	{
		m_gc_phase = Marking;

		//switch pools
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Start GC, %llu used in %llu data blocks\n", m_used, m_data_blocks);

		m_used = 0;
		m_data_blocks = 0;

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Begin Marking DFS\n");

		for(std::set<GCRoot*>::iterator it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}

		m_gc_phase = Compacting;

		forward.init(m_data_blocks); //big enough for all objects to move

		//reclaim empty blocks right off the start
		m_empty_blocks.clear();
		for(std::vector<Chunk>::iterator c = m_chunks.begin(); c != m_chunks.end(); c++)
			for(Block * b = c->begin(); b != c->end(); b = b->next())
				if(!b->m_mark)
					m_empty_blocks.push_back(b);

		if(m_empty_blocks.empty())
			alloc_chunk();

		m_cur_block = m_empty_blocks.back();
		m_empty_blocks.pop_back();

		//compact blocks that have < 80% filled
		for(std::vector<Chunk>::iterator c = m_chunks.begin(); c != m_chunks.end(); c++)
		{
			for(Block * b = c->begin(); b != c->end(); b = b->next())
			{
				if(b->m_mark && b->m_in_use < b->m_capacity*8/10)
				{
					for(Data * d = b->begin(); d != b->end(); d = d->next())
					{
						if(d->m_mark)
						{
							uchar * n = next(d->m_count, d->m_type);
							memcpy(n, d->m_data, d->m_count);
							forward.set(d->m_data, n);
						}
					}

					b->m_mark = false;
					b->m_next_alloc = 0;
					b->m_in_use = 0;
					m_empty_blocks.push_back(b);
				}
			}
		}

		m_gc_phase = Updating;

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Begin Updating DFS\n");

		for(std::set<GCRoot*>::iterator it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}


		//finishing up
		forward.clear();

		m_next_gc = std::max(CHUNK_SIZE*0.9, m_used * GC_GROWTH_LIMIT);

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Done GC, %llu used in %llu data blocks\n", m_used, m_data_blocks);

		m_gc_phase = Running;
	}
}

