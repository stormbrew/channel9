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
	GCRef<RunnableContext*> load_bytecode(Environment *env, const std::string &filename) throw(loader_error);
	// Same as above but rather than loading from the file it loads from the string given.
	GCRef<RunnableContext*> load_bytecode(Environment *env, const std::string &filename, const std::string &str) throw(loader_error);

	// Given the argv, inspects the first argument to determine what environment the program should run in
	// and then loads it, giving the full argv in the special channel of that name.
	// If given a .c9x or .so file as the first argument, the argv passed in to the script will not include
	// its own name. This is so that you can explicitly load an environment and give it the program to
	// run in the second argument.
	int load_environment_and_run(Environment *env, int argc, const char **argv) throw(loader_error);

	// Loads the file at the path given. If environment_loaded is false and the extension is unrecognized
	// it will attempt to find and load the environment first.
	int run_file(Channel9::Environment *env, const std::string &filename) throw(loader_error);
}
