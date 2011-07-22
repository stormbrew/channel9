#pragma once

#include <map>
#include <string>

#include "channel9.hpp"
#include "memory_pool.hpp"
#include "instruction.hpp"
#include "value.hpp"

namespace Channel9
{
	class Environment : private GCRoot
	{
	private:
		typedef std::map<std::string, Value> special_map;

		RunnableContext *m_context;
		bool m_running;
		special_map m_specials;

		// These put important Value data where the garbage
		// collector can find them while running an instruction.
		Instruction m_ipos;
		Value m_vstack[4]; // very important any given instruction not overload this
		size_t m_vspos;

		const Value &vstore(const Value &val) { return m_vstack[m_vspos++] = val; }
		void clear_vstore() { m_vspos = 0; }

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

