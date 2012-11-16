
#pragma once


#include <iostream>
#include <stdio.h>
#include <unistd.h>

#define VAL_TRACE_SPAM  0
#define VAL_TRACE_DEBUG 1
#define VAL_TRACE_INFO  2
#define VAL_TRACE_WARN  3
#define VAL_TRACE_ERROR 4
#define VAL_TRACE_CRIT  5
#define VAL_TRACE_OFF   6

#define VAL_TRACE_GENERAL 1
#define VAL_TRACE_VM      2
#define VAL_TRACE_GC      4
#define VAL_TRACE_ALLOC   8

#ifdef HARD_TRACE_LEVEL_GENERAL
# define HARD_TRACE_LEVEL_TRACE_GENERAL HARD_TRACE_LEVEL_GENERAL
#else
# define HARD_TRACE_LEVEL_TRACE_GENERAL 6
#endif
#ifdef HARD_TRACE_LEVEL_VM
# define HARD_TRACE_LEVEL_TRACE_VM HARD_TRACE_LEVEL_VM
#else
# define HARD_TRACE_LEVEL_TRACE_VM 6
#endif
#ifdef HARD_TRACE_LEVEL_GC
# define HARD_TRACE_LEVEL_TRACE_GC HARD_TRACE_LEVEL_GC
#else
# define HARD_TRACE_LEVEL_TRACE_GC 6
#endif
#ifdef HARD_TRACE_LEVEL_ALLOC
# define HARD_TRACE_LEVEL_TRACE_ALLOC HARD_TRACE_LEVEL_ALLOC
#else
# define HARD_TRACE_LEVEL_TRACE_ALLOC 6
#endif

#define tprintf(...) fprintf(stderr, __VA_ARGS__)
#define ctrace std::cerr

namespace Channel9
{
	extern bool trace_mute;

	void trace_out_header(int facility, int level, const char * file, int line);
}

#define TRACE_DO(facility, level) \
	if((HARD_TRACE_LEVEL_##facility) <= (VAL_##level) && !Channel9::trace_mute)

#define TRACE_OUT(facility, level) \
	TRACE_DO(facility, level) \
		Channel9::trace_out_header(VAL_##facility, VAL_##level, __FILE__, __LINE__); \
	TRACE_DO(facility, level)

#define TRACE_QUIET_CERR(facility, level, str) \
	TRACE_DO(facility, level) ctrace

#define TRACE_QUIET_PRINTF(facility, level, ...) \
	TRACE_DO(facility, level) tprintf(__VA_ARGS__)

#define TRACE_CERR(facility, level, str) \
	TRACE_OUT(facility, level) ctrace

#define TRACE_PRINTF(facility, level, ...) \
	TRACE_OUT(facility, level) tprintf(__VA_ARGS__)

