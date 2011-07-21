#pragma once

namespace Channel9
{
	// Forward declare stuff to avoid loops in headers.
	union Value;
	class Environment;
	class CallableContext;
	struct String;
	struct Tuple;
	struct RunnableContext;
	class IStream;
	class Message;
}

#if defined(TRACE)
#	define DO_TRACE if(true)
#else
#	define DO_TRACE if(false)
#endif

#if defined(TRACEGC)
#	define DO_TRACEGC if(true)
#else
#	define DO_TRACEGC if(false)
#endif

#if defined(DEBUG)
#	define DO_DEBUG if (true)
#else
#	define DO_DEBUG if (false)
#endif

