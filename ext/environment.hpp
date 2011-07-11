#pragma once

#include <map>

#include "channel9.hpp"

namespace Channel9
{
	class Environment
	{
	private:
		typedef std::map<std::string, Value> special_map;

		RunnableContext *m_context;
		bool m_running;
		special_map m_specials;

	public:
		Environment();

		const Value &special_channel(const std::string &name) const;
		void set_special_channel(const std::string &name, const Value &val);

		void run(RunnableContext *context);
		
		RunnableContext *context() const { return m_context; }
	};
}