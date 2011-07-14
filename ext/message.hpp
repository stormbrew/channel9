#pragma once

#include <string>
#include <vector>

#include "channel9.hpp"
#include "value.hpp"

namespace Channel9
{
	class Message
	{
		const std::string *m_name;
		Value::vector m_sysargs;
		Value::vector m_args;

	public:
		Message(const std::string *name, 
			const Value::vector &sysargs = Value::vector(),
			const Value::vector &args = Value::vector())
		 : m_name(name), m_sysargs(sysargs), m_args(args)
		{}
		Message(const std::string *name, size_t sysargs, size_t args)
		 : m_name(name), m_sysargs(sysargs, Value::Nil), m_args(args, Value::Nil)
		{}

		void prefix_args(const Value::vector &args);
		void prefix_arg(const Value &arg);
		void add_args(const Value::vector &args);
		void add_arg(const Value &val);

		size_t arg_count() const { return m_args.size(); }
		const std::string &name() const { return *m_name; }
		const Value::vector &args() const { return m_args; }
		const Value::vector &sysargs() const { return m_sysargs; }
		Value::vector &args() { return m_args; }
		Value::vector &sysargs() { return m_sysargs; }

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
	inline Value value(const Message &msg) MAKE_VALUE_PTR(MESSAGE, msg, new Message(msg));
	inline Value value(const Message *msg) MAKE_VALUE_PTR(MESSAGE, msg, msg);
}