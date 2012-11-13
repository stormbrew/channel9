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
	// Loads the file at the path given. If environment_loaded is false and the extension is unrecognized
	// it will attempt to find and load the environment first.
	int run_file(Channel9::Environment *env, const std::string &filename, bool environment_loaded = false) throw(loader_error);
}
