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
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Start GC, %"PRIu64" bytes used in %"PRIu64" data blocks, Begin Marking DFS\n", m_used, m_data_blocks);

		m_used = 0;
		m_data_blocks = 0;

		DO_DEBUG {
			for(std::vector<Chunk>::iterator c = m_chunks.begin(); c != m_chunks.end(); c++){
				for(Block * b = c->begin(); b != c->end(); b = b->next()){
					assert(!b->m_mark);
					for(Data * d = b->begin(); d != b->end(); d = d->next())
						assert(!d->m_mark);
				}
			}
		}

		m_dfs_marked = 0;
		m_dfs_unmarked = 0;
		m_dfs_updated = 0;

		for(std::set<GCRoot*>::iterator it = m_roots.begin(); it != m_roots.end(); it++)
		{
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan root %p\n", *it);
			(*it)->scan();
		}

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Marked %"PRIu64" objects, Begin Compacting\n", m_dfs_marked);

		m_gc_phase = Compacting;

		forward.init(m_data_blocks); //big enough for all objects to move

		//reclaim empty blocks right off the start
		m_empty_blocks.clear();
		for(std::vector<Chunk>::iterator c = m_chunks.begin(); c != m_chunks.end(); c++){
			for(Block * b = c->begin(); b != c->end(); b = b->next()){
				if(!b->m_mark){
					DO_DEBUG
						b->deadbeef();
					b->m_next_alloc = 0;
					b->m_in_use = 0;
					m_empty_blocks.push_back(b);
				}
			}
		}

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Found %u empty blocks\n", (unsigned)m_empty_blocks.size());

		//set the current allocating block to an empty one
		if(m_empty_blocks.empty())
			alloc_chunk();

		m_cur_block = m_empty_blocks.back();
		m_empty_blocks.pop_back();

		//compact blocks that have < 80% filled
		uint64_t fragmented = 0;
		uint64_t moved_bytes = 0;
		uint64_t moved_blocks = 0;
		uint64_t skipped = 0;
		for(std::vector<Chunk>::iterator c = m_chunks.begin(); c != m_chunks.end(); c++)
		{
			for(Block * b = c->begin(); b != c->end(); b = b->next())
			{
				if(b->m_mark)
				{
					if(b->m_in_use < b->m_capacity*FRAG_LIMIT)
					{
						for(Data * d = b->begin(); d != b->end(); d = d->next())
						{
							if(d->m_mark)
							{
								uchar * n = next(d->m_count, d->m_type, false);
								memcpy(n, d->m_data, d->m_count);
								forward.set(d->m_data, n);
								moved_bytes += d->m_count + sizeof(Data);
								moved_blocks++;

								//set the mark bit so it gets traversed
								Data::ptr_for(n)->m_mark = true;
							}
						}

						b->m_next_alloc = 0;
						b->m_in_use = 0;
						DO_DEBUG
							b->deadbeef();
						m_empty_blocks.push_back(b);
					}else{
						skipped++;
						fragmented += b->m_capacity - b->m_in_use;
					}
					b->m_mark = false;
				}
			}
		}

		//clear unused pinned objects
		if(m_pinned_block.m_mark)
		{
			if(m_pinned_block.m_in_use < m_pinned_block.m_capacity*FRAG_LIMIT)
			{
				std::vector<Data*> new_pinned_objs;
				for(std::vector<Data*>::iterator i = m_pinned_objs.begin(); i != m_pinned_objs.end(); ++i)
				{
					if((*i)->m_mark)
						new_pinned_objs.push_back(*i);
					else
						free(*i);
				}
				m_pinned_objs.swap(new_pinned_objs);
				m_pinned_block.m_capacity = m_pinned_block.m_in_use;
			}
			m_pinned_block.m_mark = false;
		}

		m_gc_phase = Updating;

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Done compacting, %"PRIu64" Data/%"PRIu64" bytes moved, %"PRIu64" Blocks/%"PRIu64" bytes left fragmented, Begin Updating DFS\n", moved_blocks, moved_bytes, skipped, fragmented);

		for(std::set<GCRoot*>::iterator it = m_roots.begin(); it != m_roots.end(); it++)
		{
			TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan root %p\n", *it);
			(*it)->scan();
		}

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Updated %"PRIu64" pointers, unmarked %"PRIu64" objects, cleaning up\n", m_dfs_updated, m_dfs_unmarked);

		assert(m_dfs_marked == m_dfs_unmarked);

		DO_DEBUG {
			for(std::vector<Chunk>::iterator c = m_chunks.begin(); c != m_chunks.end(); c++){
				for(Block * b = c->begin(); b != c->end(); b = b->next()){
					assert(!b->m_mark);
					for(Data * d = b->begin(); d != b->end(); d = d->next())
						assert(!d->m_mark);
				}
			}
		}


		//finishing up
		forward.clear();

		m_next_gc = std::max((1<<CHUNK_SIZE)*0.9, m_used * GC_GROWTH_LIMIT);

		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Done GC, %"PRIu64" bytes used in %"PRIu64" data blocks\n", m_used, m_data_blocks);

		m_gc_phase = Running;
	}
}

