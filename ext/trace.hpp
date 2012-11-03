
#pragma once

#define __STDC_FORMAT_MACROS

#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>

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
#define TRACE_LEVEL 4
#endif

#ifndef TRACE_SUB
#define TRACE_SUB ((1<<30)-1) //subscribe to everything by default
#endif

#define tprintf(...) fprintf(stderr, __VA_ARGS__)
#define ctrace std::cerr

inline void trace_out_header(int facility, int level, const char * file, int line){
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

	if(color) ctrace << cyan;
	switch(facility){
		case TRACE_GENERAL: ctrace << "general";   break;
		case TRACE_VM:      ctrace << "vm";        break;
		case TRACE_GC:      ctrace << "gc";        break;
		default: ctrace << "unknown " << facility; break;
	}
	if(color) ctrace << reset;

	ctrace << ".";

	if(color) ctrace << levelcolor[level];
	switch(level){
		case TRACE_SPAM:  ctrace << "spam";     break;
		case TRACE_DEBUG: ctrace << "debug";    break;
		case TRACE_INFO:  ctrace << "info";     break;
		case TRACE_WARN:  ctrace << "warn";     break;
		case TRACE_ERROR: ctrace << "error";    break;
		case TRACE_CRIT:  ctrace << "critical"; break;
		default:          ctrace << "unknown";  break;
	}

	if(color) ctrace << blue;
	ctrace << " " << file << ":" << line << ": "; \
	if(color) ctrace << reset;
}

#define TRACE_QUIET_OUT(facility, level) \
	if(((TRACE_SUB) & (facility)) && (TRACE_LEVEL) <= (level))

#define TRACE_QUIET_CERR(facility, level, str) \
	TRACE_QUIET_OUT(facility, level) ctrace

#define TRACE_QUIET_PRINTF(facility, level, ...) \
	TRACE_QUIET_OUT(facility, level) tprintf(__VA_ARGS__)

#define TRACE_OUT(facility, level) \
	TRACE_QUIET_OUT(facility, level){ \
		trace_out_header(facility, level, __FILE__, __LINE__); \
	} \
	TRACE_QUIET_OUT(facility, level)

#define TRACE_CERR(facility, level, str) \
	TRACE_OUT(facility, level) ctrace

#define TRACE_PRINTF(facility, level, ...) \
	TRACE_OUT(facility, level) tprintf(__VA_ARGS__)

