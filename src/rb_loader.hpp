#pragma once

#include "c9/callable_context.hpp"

namespace Channel9
{
	class LoaderChannel : public CallableContext
	{
	private:
		std::string environment_path;
		uint64_t mid_load_c9;
		uint64_t mid_load;
		uint64_t mid_compile;
		uint64_t mid_backtrace;

	public:
		LoaderChannel(const std::string &environment_path);

		void send(Channel9::Environment *env, const Channel9::Value &val, const Channel9::Value &ret);

		std::string inspect() const
		{
			return "Ruby Loader Channel";
		}
	};
}
