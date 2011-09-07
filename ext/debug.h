
#pragma once

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#define DEBUG_SPAM  0
#define DEBUG_DEBUG 1
#define DEBUG_INFO  2
#define DEBUG_WARN  3
#define DEBUG_ERROR 4
#define DEBUG_CRIT  5

#define DEBUG_GENERAL 1
#define DEBUG_VM      2
#define DEBUG_GC      4


#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL 4
#endif

#ifndef DEBUG_SUB
#define DEBUG_SUB ((1<<30)-1) //subscribe to everything by default
#endif

inline void debug_out_header(int facility, int level, const char * file, int line){
	static const char
		*black     = "\033[30m",
		*red       = "\033[31m",
		*green     = "\033[32m",
		*yellow    = "\033[33m",
		*blue      = "\033[34m",
		*purple    = "\033[35m",
		*cyan      = "\033[36m",
		*white     = "\033[37m",
		*bold      = "\033[1m",
		*underline = "\033[4m",
		*reset     = "\033[0m";

	static const char * levelcolor[] = { white, white, green, yellow, red, red };

	static bool color = isatty(2); //stderr

	if(color) std::cerr << cyan;
	switch(facility){
		case DEBUG_GENERAL: std::cerr << "general";   break;
		case DEBUG_VM:      std::cerr << "vm";        break;
		case DEBUG_GC:      std::cerr << "gc";        break;
		default: std::cerr << "unknown " << facility; break;
	}
	if(color) std::cerr << reset;

	std::cerr << ".";

	if(color) std::cerr << levelcolor[level];
	switch(level){
		case DEBUG_SPAM:  std::cerr << "spam";     break;
		case DEBUG_DEBUG: std::cerr << "debug";    break;
		case DEBUG_INFO:  std::cerr << "info";     break;
		case DEBUG_WARN:  std::cerr << "warn";     break;
		case DEBUG_ERROR: std::cerr << "error";    break;
		case DEBUG_CRIT:  std::cerr << "critical"; break;
		default:          std::cerr << "unknown";  break;
	}

	if(color) std::cerr << blue;
	std::cerr << " " << file << ":" << line << ": "; \
	if(color) std::cerr << reset;
}

#define DEBUG_OUT(facility, level, code) \
	if(((DEBUG_SUB) & (facility)) && (DEBUG_LEVEL) <= (level)){ \
		debug_out_header(facility, level, __FILE__, __LINE__); \
		code \
	}

#define DEBUG_CERR(facility, level, str) \
	DEBUG_OUT(facility, level, std::cerr << str; )

#define DEBUG_PRINTF(facility, level, ...) \
	DEBUG_OUT(facility, level, fprintf(stderr, __VA_ARGS__); )

