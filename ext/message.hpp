#pragma once

#include <string>
#include <vector>

#include "channel9.hpp"
#include "value.hpp"

namespace Channel9
{
	struct Message
	{
		const std::string *m_name;
		size_t m_sysarg_count;
		size_t m_arg_count;

		Value m_data[0];

		typedef Value *iterator;
		typedef const Value *const_iterator;

		size_t sysarg_count() const { return m_sysarg_count; }
		size_t arg_count() const { return m_arg_count; }
		size_t total_count() const { return m_sysarg_count + m_arg_count; }

		const std::string &name() const { return *m_name; }
		const_iterator args() const { return m_data + m_sysarg_count; }
		const_iterator args_end() const { return m_data + m_sysarg_count + m_arg_count; }
		const_iterator sysargs() const { return m_data; }
		const_iterator sysargs_end() const { return m_data + m_sysarg_count; }

		iterator args() { return m_data + m_sysarg_count; }
		iterator args_end() { return m_data + m_sysarg_count + m_arg_count; }
		iterator sysargs() { return m_data; }
		iterator sysargs_end() { return m_data + m_sysarg_count; }
	};

	inline Message *new_message(const std::string *name, size_t sysargs = 0, size_t args = 0)
	{
		size_t count = sysargs + args;
		Message *msg = value_pool.alloc<Message>(sizeof(Value)*count);
		msg->m_name = name;
		msg->m_sysarg_count = sysargs;
		msg->m_arg_count = args;
		return msg;
	}
	template <typename tIter>
	inline Message *new_message(const std::string *name, size_t sysargs, tIter sysarg_it, size_t args, tIter arg_it)
	{
		Message *msg = new_message(name, sysargs, args);
		Value *out = msg->m_data;
		while (sysargs != 0)
		{
			*out++ = *sysarg_it++;
			--sysargs;
		}
		while (args != 0)
		{
			*out++ = *arg_it++;
			--args;
		}
		return msg;
	}
	inline Message *new_message(const Message &other)
	{
		size_t size = sizeof(Message) + sizeof(Value)*other.total_count();
		Message *msg = value_pool.alloc<Message>(sizeof(Value)*other.total_count());
		memcpy(msg, &other, size);
		return msg;
	}

	inline Value value(const Message &msg) MAKE_VALUE_PTR(MESSAGE, msg, new_message(msg));
	inline Value value(const Message *msg) MAKE_VALUE_PTR(MESSAGE, msg, msg);
}