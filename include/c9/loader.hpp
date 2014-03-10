#pragma once
#include "c9/environment.hpp"

namespace Channel9
{
	class loader_error : public std::exception
	{
	public:
		std::string reason;

		loader_error(const std::string &reason) : reason(reason) {}
		~loader_error() throw() {}
	};

	// Loads the bytecode file at the path given. Returns a runnablecontext that can be activated.
	GCRef<RunnableContext*> load_bytecode(Environment *env, const std::string &filename);
	// Same as above but rather than loading from the file it loads from the string given.
	GCRef<RunnableContext*> load_bytecode(Environment *env, const std::string &filename, const std::string &str);

	// Given the program name and argv, inspects the program name or first argument to determine
	// which environment to load (based on either the program being c9.ENVNAME or the extension
	// of the first argument being ENVNAME.
	// It then loads the environment, giving the full argv in the special channel of that name.
	// If given a .c9x or .so file as the first argument, the argv passed in to the script will not include
	// its own name. This is so that you can explicitly load an environment and give it the program to
	// run in the second argument.
	// Will also set a trace_loaded special channel in the environment to the value of trace_loaded.
	// This is to advise the environment that it should unmute tracing before acting on its argv.
	int load_environment_and_run(Environment *env, std::string program, int argc, const char **argv, bool trace_loaded, bool compile);

	// Loads the file at the path given. If environment_loaded is false and the extension is unrecognized
	// it will attempt to find and load the environment first.
	int run_file(Channel9::Environment *env, const std::string &filename);
}
