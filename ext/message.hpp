#pragma once

#include <string>
#include <vector>

#include "channel9.hpp"
#include "value.hpp"

namespace Channel9
{
	class Message
	{
		std::string m_name;
		Value::vector m_sysargs;
		Value::vector m_args;

	public:
		Message(const std::string &name, 
			const Value::vector &sysargs = Value::vector(),
			const Value::vector &args = Value::vector());

		void add_args(const Value::vector &args);
		void add_arg(const Value &val);

		size_t arg_count() const { return m_args.size(); }
		const std::string &name() const { return m_name; }
		const Value::vector &args() const { return m_args; }
		const Value::vector &sysargs() const { return m_sysargs; }

		typedef Value::vector::iterator iterator;
		typedef Value::vector::const_iterator const_iterator;

		iterator begin_sys() { return m_sysargs.begin(); }
		iterator end_sys() { return m_sysargs.end(); }
		const_iterator begin_sys() const { return m_sysargs.begin(); }
		const_iterator end_sys() const { return m_sysargs.end(); }

		iterator begin() { return m_args.begin(); }
		iterator end() { return m_args.end(); }
		const_iterator begin() const { return m_args.begin(); }
		const_iterator end() const { return m_args.end(); }
	};
}