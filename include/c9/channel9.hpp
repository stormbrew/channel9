#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

namespace Channel9
{
	// Forward declare stuff to avoid loops in headers.
	union Value;
	class Environment;
	class CallableContext;
	struct String;
	struct Tuple;
	struct RunningContext;
	struct RunnableContext;
	struct VariableFrame;
	struct Message;
	class IStream;

	enum ValueType
	{
		POSITIVE_NUMBER = 0x00,

		BTRUE			= 0x10,
		FLOAT_NUM 		= 0x20,

		// these are types that will eventually
		// be made into full heap types, but for now
		// are allocated on the normal heap so need
		// to be treated as plain old values.
		CALLABLE_CONTEXT= 0x40,

		// Bit pattern of 0000 1000 indicates falsy,
		// all other type patterns are truthy.
		FALSY_MASK 		= 0xCF,
		FALSY_PATTERN 	= 0x80,

		NIL 			= 0x80,
		UNDEF 			= 0x90,
		BFALSE 			= 0xA0,
		// don't use 	= 0xB0,

		// All heap types have E as their high nibble,
		// as that nibble comes from the reference itself
		// and indicates that the full type is in the
		// next nibble of the address
		HEAP_TYPE 		= 0xE0,

		VARIABLE_FRAME  = 0xE2,
		STRING          = 0xE3,
		TUPLE           = 0xE4,
		MESSAGE         = 0xE5,
		RUNNABLE_CONTEXT= 0xE7,
		RUNNING_CONTEXT = 0xE8,

		NEGATIVE_NUMBER = 0xF0
	};

	// this really belongs in value.hpp, but header load order
	// makes that impractical. It needs to be there for the GC
	// stuff, which value.hpp depends on.
	// TODO: Fix that load order and put this back where it belongs.
	enum {
		POINTER_MASK 	= 0x00007fffffffffffULL,
	};
	// get just the pointer portion of something that may be a Value
	template <typename tType>
	void *raw_tagged_ptr(tType &ref)
	{
		tType *ptr = &ref;
		return (void*)(*(uintptr_t*)ptr & POINTER_MASK);
	}
	// update just the pointer portion of something that may be a Value
	template <typename tType>
	void update_tagged_ptr(tType *ptr, const void *to)
	{
		uintptr_t &ref = *(uintptr_t*)ptr;
		ref = ((uintptr_t)to & POINTER_MASK) | (ref & ~POINTER_MASK);
	}

	/* On some platforms (*cough* OSX/x64 *cough*), uintptr_t doesn't map
	 * to the same type as any of the uint*_t types (uint64 is long long and
	 * uintptr_t is long), and this makes function overloading just plain not
	 * work. To get around this, we redefine uintptr_t for the channel9
	 * namespace so it matches one of those types.
	 */
	template <size_t ptrsize_t>
	struct ptrsize;
	template <>
	struct ptrsize<4>
	{
		typedef uint32_t uptrtype_t;
		typedef int32_t ptrtype_t;
	};
	template <>
	struct ptrsize<8>
	{
		typedef uint64_t uptrtype_t;
		typedef int64_t ptrtype_t;
	};
	typedef ptrsize<sizeof(void*)>::uptrtype_t uintptr_t;
	typedef ptrsize<sizeof(void*)>::ptrtype_t intptr_t;
}

#include "c9/trace.hpp"

#if defined(DEBUG)
#	define DO_DEBUG if (true)
#else
#	define DO_DEBUG if (false)
#endif

#if defined(__GNUC__)
#	define likely(x) __builtin_expect(!!(x), 1)
#	define unlikely(x) __builtin_expect(!!(x), 0)
#else
#	define likely(x) (x)
#	define unlikely(x) (x)
#endif
