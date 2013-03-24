#pragma once

#include "c9/callable_context.hpp"

namespace Channel9
{
	class RegexpChannel : public CallableContext
	{
	public:
		RegexpChannel(){}
		~RegexpChannel(){}

		void send(Environment *env, const Value &val, const Value &ret);
		std::string inspect() const
		{
			return "Regexp Generator Channel";
		}
	};
}
