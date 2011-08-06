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
}

