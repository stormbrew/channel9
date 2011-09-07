#pragma once

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
	class IStream;
	class Message;

	enum ValueType
	{
		POSITIVE_NUMBER = 0x00,

		BTRUE			= 0x01,
		FLOAT_NUM 		= 0x02,

		// these are types that will eventually
		// be made into full heap types, but for now
		// are allocated on the normal heap so need
		// to be treated as plain old values.
		CALLABLE_CONTEXT= 0x04,
		VARIABLE_FRAME  = 0x05,

		// Bit pattern of 0000 1000 indicates falsy,
		// all other type patterns are truthy.
		FALSY_MASK 		= 0xFC,
		FALSY_PATTERN 	= 0x08,

		NIL 			= 0x08,
		UNDEF 			= 0x09,
		BFALSE 			= 0x0A,
		// don't use 	= 0x0B,

		// All heap types have E as their low nibble,
		// as that nibble comes from the reference itself
		// and indicates that the full type is in the
		// object referred to.
		HEAP_TYPE 		= 0x0E,

		STRING          = 0x3E,
		TUPLE           = 0x4E,
		MESSAGE         = 0x5E,
		RUNNABLE_CONTEXT= 0x7E,
		RUNNING_CONTEXT = 0x8E,

		NEGATIVE_NUMBER = 0x0f
	};
}

#include "debug.h"

#if defined(DEBUG)
#	define DO_DEBUG if (true)
#else
#	define DO_DEBUG if (false)
#endif

