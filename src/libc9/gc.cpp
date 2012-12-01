#include "c9/channel9.hpp"
#include "c9/gc.hpp"
#include "c9/gc_nursery.hpp"


namespace Channel9
{
	GC::Tenure tenure_pool;
	GC::Nursery nursery_pool(8*1024*1024, GC::GEN_NURSERY, &tenure_pool);

	scan_func *scan_types[0xff];

	std::set<GCRoot*> GC::m_roots;
	uint8_t GC::collecting_generation = -1;

	void GC::register_root(GCRoot *root)
	{
		m_roots.insert(root);
	}
	void GC::unregister_root(GCRoot *root)
	{
		m_roots.erase(root);
	}

	GCRoot::GCRoot()
	{
		GC::register_root(this);
	}

	GCRoot::~GCRoot()
	{
		GC::unregister_root(this);
	}
}

