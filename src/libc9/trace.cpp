#include "c9/trace.hpp"
#include <stdio.h>
#include <iomanip>
#include "c9/time.hpp"

namespace Channel9
{
	bool trace_mute = false;

	Time start_time;

	void trace_out_header(int facility, int level, const char * file, int line)
	{
		static const char
	//		*black     = "\033[30m",
			*red       = "\033[31m",
			*green     = "\033[32m",
			*yellow    = "\033[33m",
			*blue      = "\033[34m",
	//		*purple    = "\033[35m",
			*cyan      = "\033[36m",
			*white     = "\033[37m",
	//		*bold      = "\033[1m",
	//		*underline = "\033[4m",
			*reset     = "\033[0m";

		static const char * levelcolor[] = { white, white, green, yellow, red, red };

		static bool color = isatty(2); //stderr

		if(color) ctrace << white;
		ctrace << std::setprecision(3) << std::fixed << (Time() - start_time) << " ";

		if(color) ctrace << cyan;
		switch(facility)
		{
			case VAL_TRACE_GENERAL: ctrace << "general";   break;
			case VAL_TRACE_VM:      ctrace << "vm";        break;
			case VAL_TRACE_GC:      ctrace << "gc";        break;
			case VAL_TRACE_ALLOC:   ctrace << "alloc";     break;
			default: ctrace << "unknown " << facility; break;
		}
		if(color) ctrace << reset;

		ctrace << ".";

		if(color) ctrace << levelcolor[level];
		switch(level)
		{
			case VAL_TRACE_SPAM:  ctrace << "spam";     break;
			case VAL_TRACE_DEBUG: ctrace << "debug";    break;
			case VAL_TRACE_INFO:  ctrace << "info";     break;
			case VAL_TRACE_WARN:  ctrace << "warn";     break;
			case VAL_TRACE_ERROR: ctrace << "error";    break;
			case VAL_TRACE_CRIT:  ctrace << "critical"; break;
			default:          ctrace << "unknown";  break;
		}

		if(color) ctrace << blue;
		ctrace << " " << file << ":" << line << ": "; \
		if(color) ctrace << reset;
	}
}
