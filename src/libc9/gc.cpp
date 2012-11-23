#include "c9/channel9.hpp"
#include "c9/gc.hpp"
#include "c9/gc_nursery.hpp"


namespace Channel9
{
	GC::Nursery nursery_pool;
	GC::Normal normal_pool;
	//GC::Tenure tenure_pool;
	scan_func *scan_types[0xff];

	void GC::scan(void *ptr, ValueType type)
	{
		TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan Obj %p, type %x\n", ptr, type);
		if (scan_types[type])
			scan_types[type](ptr);
	}

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

