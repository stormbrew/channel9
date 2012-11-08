#include "c9/channel9.hpp"
#include "c9/memory_pool.hpp"
#include "c9/gc_nursery.hpp"


namespace Channel9
{
	GC::Nursery<MemoryPool> value_pool;

	GCRoot::GCRoot(GC::Nursery<MemoryPool> &pool)
	 : m_pool(pool)
	{
		m_pool.register_root(this);
	}

	GCRoot::~GCRoot()
	{
		m_pool.unregister_root(this);
	}
}

