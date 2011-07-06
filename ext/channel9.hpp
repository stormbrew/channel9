#pragma once

namespace Channel9
{
	// Forward declare stuff to avoid loops in headers.
	struct Value;
	class Environment;
	class CallableContext;
	class RunnableContext;
	class IStream;
	class Message;
}

#if 0
#	define DO_DEBUG if(true)
#else
#	define DO_DEBUG if(false)
#endif