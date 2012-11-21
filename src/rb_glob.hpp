#pragma once

#include "c9/callable_context.hpp"

namespace Channel9
{
	class GlobChannel : public CallableContext
	{
	public:
		GlobChannel(){}
		~GlobChannel(){}

		void send(Environment *env, const Value &val, const Value &ret);
		std::string inspect() const
		{
			return "Glob Channel";
		}
	};
}
