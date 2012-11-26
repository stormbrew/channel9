#include "c9/channel9.hpp"
#include "c9/gc.hpp"
#include "c9/gc_nursery.hpp"


namespace Channel9
{
	GC::Nursery nursery_pool;
	GC::Normal normal_pool;
	//GC::Tenure tenure_pool;
	scan_func *scan_types[0xff];

	GCRoot::GCRoot(GC::Nursery &pool)
	 : m_pool(pool)
	{
		m_pool.register_root(this);
	}

	GCRoot::~GCRoot()
	{
		m_pool.unregister_root(this);
	}
}

