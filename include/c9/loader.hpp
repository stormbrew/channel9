#pragma once
#include "c9/environment.hpp"

namespace Channel9
{
	// Loads the file at the path given. If environment_loaded is false and the extension is unrecognized
	// it will attempt to find and load the environment first.
	int run_file(const char *program, Channel9::Environment *env, const char *filename, bool environment_loaded = false);
}
