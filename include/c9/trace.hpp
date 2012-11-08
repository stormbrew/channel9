
#pragma once


#include <iostream>
#include <stdio.h>
#include <unistd.h>

#define TRACE_SPAM  0
#define TRACE_DEBUG 1
#define TRACE_INFO  2
#define TRACE_WARN  3
#define TRACE_ERROR 4
#define TRACE_CRIT  5

#define TRACE_GENERAL 1
#define TRACE_VM      2
#define TRACE_GC      4
#define TRACE_ALLOC   8


#ifndef TRACE_LEVEL
#define TRACE_LEVEL TRACE_ERROR
#endif

#ifndef TRACE_SUB
#define TRACE_SUB ((1<<30)-1) //subscribe to everything by default
#endif

#define tprintf(...) fprintf(stderr, __VA_ARGS__)
#define ctrace std::cerr

namespace Channel9
{
	extern bool trace_mute;

	void trace_out_header(int facility, int level, const char * file, int line);
}

#define TRACE_DO(facility, level) \
	if(((TRACE_SUB) & (facility)) && (TRACE_LEVEL) <= (level) && !Channel9::trace_mute)

#define TRACE_OUT(facility, level) \
	TRACE_DO(facility, level) \
		Channel9::trace_out_header(facility, level, __FILE__, __LINE__); \
	TRACE_DO(facility, level)

#define TRACE_QUIET_CERR(facility, level, str) \
	TRACE_DO(facility, level) ctrace

#define TRACE_QUIET_PRINTF(facility, level, ...) \
	TRACE_DO(facility, level) tprintf(__VA_ARGS__)

#define TRACE_CERR(facility, level, str) \
	TRACE_OUT(facility, level) ctrace

#define TRACE_PRINTF(facility, level, ...) \
	TRACE_OUT(facility, level) tprintf(__VA_ARGS__)

