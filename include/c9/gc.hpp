#pragma once

#include <set>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include <stdio.h>

#include "c9/channel9.hpp"
#include "memcheck.h"


namespace Channel9
{
	class GCRoot;

	class GC { // GC base class, must be subclassed by one of the others
		//small allocations are smaller than SMALL, medium is smaller than MEDIUM, everything bigger is big
		//these are defaults, may be overwritten by the collectors themselves, or even ignored entirely
		//small is the maximum amount of memory we want to just be wasting if we can't fit the current allocation into the current block/chunk
		//medium is the point where we may want to switch to a different allocation strategy to minimize moving large objects
		//both are intentionally chosen as values a little below powers of 2 to leave room for block/chunk headers
		static const unsigned int SMALL = 4000;
		static const unsigned int MEDIUM = 32700;

		// notify the gc that an object is pointed to by the pointer in from which is
		// in object obj.
		// might mark it, might move it, might do something else. Returns true if it moved
		// if obj is NULL, the pointer is from outside the collector's control.
		bool mark(void *obj, uintptr_t *from);

		// is this object valid? only to be used for debugging
		template <typename tObj> bool validate(tObj * obj);

		// make sure this object is ready to be read from
		template <typename tObj> void read_ptr(tObj * obj);

		// safely write val to field in obj. If obj is NULL,
		// the location is outside the control of the garbage
		// collector.
		template <typename tField, typename tVal>
		void write_ptr(void *obj, tField &field, const tVal &val);

		static std::set<GCRoot*> m_roots;

		static uint8_t collecting_generation;

	protected:
		// scans a garbage collected object of type. Calls
		// the nursery collector's mark function on any references
		// inside.
		inline void scan(void *ptr, ValueType obj);

	public:
		static const uint8_t GEN_NURSERY = 0;
		static const uint8_t GEN_TENURE  = 2;
		static const uint8_t GEN_MASK    = 0x3;

		struct GenericData
		{
			// Before every object tracked by a GC there must be
			// an 8-byte header with this structure in its first
			// 6 bytes.
			uint32_t m_bytes;      // the number of bytes after the data header
			uint8_t  m_generation; // the generation this object is from
			uint8_t  m_type;       // the ValueType of this object.

			// The last two bytes are type-specific. If more is needed,
			// there can be extra data put before this header.
			uint16_t   m_flags; // flags specific to the collector.
		};

		class Semispace;
		class Markcompact;

		class Nursery;

		typedef COLLECTOR_CLASS Tenure;

		// called by the next generation down to promote an
		// already allocated object to the generation in question.
		virtual void *promote(void *obj, size_t size, ValueType type, bool pinned) = 0;
		// called by the next generation down to allocate an
		// object is the current generation can't.
		virtual void *raw_alloc(size_t size, ValueType type, bool pinned) = 0;

		// Scans a garbage collected object at ptr. Assumes
		// it's prefixed by a valid GenericData struct.
		inline void scan(void *ptr);

		static inline void safe_point();

		static void register_root(GCRoot *root);
		static void unregister_root(GCRoot *root);

		template <typename tObj>
		friend bool gc_mark(void *in, tObj **obj);
	};

	extern GC::Nursery nursery_pool;
	extern GC::Tenure tenure_pool;

	// Base class for GC root objects. Should inherit privately.
	class GCRoot
	{
	protected:
		GCRoot();

	public:
		virtual void scan() = 0;

		virtual ~GCRoot();
	};
	// returns the generic object header of a garbage collected
	// pointer.
	inline GC::GenericData &header_of(const void *ptr)
	{
		return *(((GC::GenericData*)ptr)-1);
	}
	inline uint8_t generation_of(const void *ptr)
	{
		return header_of(ptr).m_generation;
	}
	inline bool is_generation(const void *ptr, uint8_t generation)
	{
		return header_of(ptr).m_generation == generation;
	}
	inline bool is_nursery(const void *ptr)
	{
		return is_generation(ptr, GC::GEN_NURSERY);
	}
	inline bool is_tenure(const void *ptr)
	{
		return is_generation(ptr, GC::GEN_TENURE);
	}

	typedef void (scan_func)(void *obj);
	extern scan_func *scan_types[0xff];

#	define INIT_SCAN_FUNC(type, func) \
		static scan_func *_scan_func_##type = scan_types[type] = (scan_func*)func;
}

#include "c9/gc_semispace.hpp"
#include "c9/gc_markcompact.hpp"
#include "c9/gc_nursery.hpp"

namespace Channel9
{
	inline void *gc_alloc(size_t size, ValueType type, bool pinned = false)
	{
		// note that this is a call to a virtual function,
		// but it should not be invoked as such in an optimized
		// build since the type is known.
		return nursery_pool.raw_alloc(size, type, pinned);
	}

	template <typename tObj>
	tObj *gc_alloc(size_t extra, bool pinned = false)
	{
		return reinterpret_cast<tObj*>(gc_alloc(sizeof(tObj) + extra, tObj::gc_type_id, pinned));
	}

	template <typename tObj>
	bool gc_mark(void *in, tObj **obj)
	{
		switch (GC::collecting_generation)
		{
		case GC::GEN_NURSERY:
			return nursery_pool.mark(in, (uintptr_t*)obj);
		case GC::GEN_TENURE:
			return tenure_pool.mark(in, (uintptr_t*)obj);
		default:
			assert(false && "Mark without collecting.");
			return false;
		}
	}

	// safely write val to field in obj. If obj is NULL,
	// the location is outside the control of the garbage
	// collector.
	template <typename tField, typename tVal>
	void gc_write_ptr(void *obj, tField &field, const tVal &val)
	{
		nursery_pool.write_ptr(obj, field, val);
		tenure_pool.write_ptr(obj, field, val);
		field = val;
	}

	void GC::scan(void *ptr, ValueType type)
	{
		TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Scan Obj %p, type %x\n", ptr, type);
		if (scan_types[type])
			scan_types[type](ptr);
	}
	void GC::scan(void *ptr)
	{
		assert(sizeof(GenericData) == 8);
		scan(ptr, (ValueType)header_of(ptr).m_type);
	}

	inline void GC::safe_point()
	{
		bool nursery = nursery_pool.need_collect(),
			 tenure = tenure_pool.need_collect();

		nursery = nursery || tenure;

		collecting_generation = GEN_NURSERY;
		if (nursery) nursery_pool.collect();
		collecting_generation = GEN_TENURE;
		if (tenure) tenure_pool.collect();
		collecting_generation = -1;
	}
}

#undef COLLECTOR
#undef COLLECTOR_CLASS

