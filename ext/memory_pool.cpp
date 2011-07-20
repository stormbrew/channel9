#include "channel9.hpp"
#include "memory_pool.hpp"

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
		m_cur_pool = !m_cur_pool;
		m_cur_chunk = m_pools[m_cur_pool];

		//clear this pool
		Chunk * c = m_cur_chunk;
		while(c){
			c->m_used = 0;
			c = c->m_next;
		}

		std::set<GCRoot*>::iterator it;
		for (it = m_roots.begin(); it != m_roots.end(); it++)
		{
			(*it)->scan();
		}

		//copy stuff over?

		m_in_gc = false;
	}


}