#pragma once

#include <set>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

#include "channel9.hpp"
#include "memcheck.h"


namespace Channel9
{
	class GCRoot;

	class GC { // GC base class, must be subclassed by one of the others
		//small allocations are smaller than SMALL, medium is smaller than MEDIUM, everything bigger is big
		//these are defaults, may be overwritten by the collectors themselves, or even ignored entirely
		static const unsigned int SMALL = 500;
		static const unsigned int MEDIUM = 8000;

		// extra is how much to allocate, type is one of Channel9::ValueType, return the new location
		template <typename tObj> tObj *alloc(size_t extra, uint16_t type, bool pinned = false);

		template <typename tObj> tObj *alloc_small (size_t extra, uint16_t type);
		template <typename tObj> tObj *alloc_med   (size_t extra, uint16_t type);
		template <typename tObj> tObj *alloc_big   (size_t extra, uint16_t type);
		template <typename tObj> tObj *alloc_pinned(size_t extra, uint16_t type);


		// notify the gc that an obj is pointed to, might mark it, might move it, might do something else. Returns true if it moved
		template <typename tObj> bool mark(tObj ** from);

		// is this object valid? only to be used for debugging
		template <typename tObj> bool validate(tObj * obj);

		// make sure this object is ready to be read from
		template <typename tObj> void read_barrier(tObj * obj);

		// tell the GC that obj will contain a reference to the object pointed to by ptr
		template <typename tObj, typename tPtr> void write_barrier(tObj * obj, tPtr * ptr);

		void safe_point(); // now is a valid time to stop the world

		void register_root(GCRoot *root);
		void unregister_root(GCRoot *root);

	public:
		class Semispace;
		class Markcompact;
	};

	typedef GC::COLLECTOR_CLASS MemoryPool;

	extern MemoryPool value_pool;

	// Base class for GC root objects. Should inherit privately.
	class GCRoot
	{
	private:
		MemoryPool &m_pool;

	protected:
		GCRoot(MemoryPool &pool);

		MemoryPool &pool() const { return m_pool; }

	public:
		virtual void scan() = 0;

		virtual ~GCRoot();
	};
}

#include "gc_semispace.hpp"
#include "gc_markcompact.hpp"

#undef COLLECTOR
#undef COLLECTOR_CLASS

