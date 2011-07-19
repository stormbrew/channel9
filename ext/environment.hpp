#pragma once

#include <map>

#include "channel9.hpp"
#include "memory_pool.hpp"

namespace Channel9
{
	class Environment : private GCRoot
	{
	private:
		typedef std::map<std::string, Value> special_map;

		RunnableContext *m_context;
		bool m_running;
		special_map m_specials;

	public:
		Environment();

		const Value &special_channel(const std::string &name) const;
		const Value &special_channel(const String &name) const;
		void set_special_channel(const std::string &name, const Value &val);

		void run(RunnableContext *context);

		void scan();
		
		RunnableContext *context() const { return m_context; }
	};
}