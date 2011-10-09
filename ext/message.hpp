#pragma once

#include <map>
#include <vector>

#include "channel9.hpp"
#include "value.hpp"

namespace Channel9
{
	typedef std::map<std::string, uint64_t> message_id_map;
	typedef std::vector<std::string> message_name_list;
	extern message_id_map message_ids;
	extern message_name_list message_names;
	extern message_name_list protocol_names;

	enum {
		PROTOCOL_ID_MASK = 0x00ff000000000000,
		MESSAGE_ID_MASK  = 0x0000ffffffffffff,
		PROTOCOL_ID_SHIFT = 48,

		PROTOCOL_DEFAULT = 0,
		PROTOCOL_C9_SYSTEM = 1,

		MESSAGE_NOP = 0,

		MESSAGE_PLUS,
		MESSAGE_MINUS,
		MESSAGE_MULTIPLY,
		MESSAGE_DIVIDE,
		MESSAGE_BITWISE_AND,
		MESSAGE_BITWISE_OR,
		MESSAGE_MODULUS,
		MESSAGE_LT,
		MESSAGE_GT,
		MESSAGE_EQUAL,
		MESSAGE_LT_EQUAL,
		MESSAGE_GT_EQUAL,
		MESSAGE_POWER,
		MESSAGE_NEGATE,

		MESSAGE_TO_STRING_PRIMITIVE,
		MESSAGE_TO_NUM_PRIMITIVE,
		MESSAGE_TO_FLOAT_PRIMITIVE,
		MESSAGE_TO_TUPLE_PRIMITIVE,
		MESSAGE_TO_CHR,
		MESSAGE_TO_MESSAGE_ID,
		MESSAGE_TO_PROTOCOL_ID,
		MESSAGE_TO_MESSAGE_NAME,
		MESSAGE_TO_PROTOCOL_NAME,

		MESSAGE_HASH,

		MESSAGE_SPLIT,
		MESSAGE_SUBSTR,
		MESSAGE_COMPARE,
		MESSAGE_LENGTH,

		MESSAGE_AT,
		MESSAGE_SUBARY,
		MESSAGE_PUSH,
		MESSAGE_POP,
		MESSAGE_FRONT_PUSH,
		MESSAGE_FRONT_POP,
		MESSAGE_REPLACE,
		MESSAGE_DELETE,
		MESSAGE_FIRST,
		MESSAGE_LAST,

		MESSAGE_NAME,
		MESSAGE_UNFORWARD,

		MESSAGE_C9_PRIMITIVE_CALL,
	};

	inline uint64_t make_protocol_id(uint64_t proto, uint64_t msg)
	{
		return (proto << PROTOCOL_ID_SHIFT) | msg;
	}
	uint64_t make_protocol_id(const std::string &name);
	uint64_t make_protocol_id(const String *name);
	uint64_t make_message_id(const std::string &name);
	uint64_t make_message_id(const String *name);

	struct Message
	{
		union {
			uint64_t m_id;
			struct {
				uint64_t m_message_id:48;
				uint64_t m_protocol_id:8;
				uint64_t m_reserved_id:8;
			};
		};
		uint32_t m_sysarg_count;
		uint32_t m_arg_count;

		Value m_data[0];

		typedef Value *iterator;
		typedef const Value *const_iterator;

		uint32_t sysarg_count() const { return m_sysarg_count; }
		uint32_t arg_count() const { return m_arg_count; }
		uint32_t total_count() const { return m_sysarg_count + m_arg_count; }

		uint64_t message_id() const { return m_id & MESSAGE_ID_MASK; }
		uint64_t protocol_id() const { return (m_id & PROTOCOL_ID_MASK) >> PROTOCOL_ID_SHIFT; }
		const std::string &protocol() const;
		const std::string &simple_name() const; // just the message name without protocol prefix.
		std::string name() const;

		const_iterator args() const { return m_data + m_sysarg_count; }
		const_iterator args_end() const { return m_data + m_sysarg_count + m_arg_count; }
		const_iterator sysargs() const { return m_data; }
		const_iterator sysargs_end() const { return m_data + m_sysarg_count; }

		iterator args() { return m_data + m_sysarg_count; }
		iterator args_end() { return m_data + m_sysarg_count + m_arg_count; }
		iterator sysargs() { return m_data; }
		iterator sysargs_end() { return m_data + m_sysarg_count; }
	};

	inline Message *new_message(uint64_t message_id, size_t sysargs = 0, size_t args = 0)
	{
		size_t count = sysargs + args;
		Message *msg = value_pool.alloc<Message>(sizeof(Value)*count, MESSAGE);
		msg->m_id = message_id;
		msg->m_sysarg_count = sysargs;
		msg->m_arg_count = args;
		return msg;
	}
	inline Message *new_message(const String *name, size_t sysargs = 0, size_t args = 0)
	{
		return new_message(make_message_id(name), sysargs, args);
	}
	inline Message *new_message(const Value &name, size_t sysargs = 0, size_t args = 0)
	{
		return new_message(make_message_id(ptr<String>(name)), sysargs, args);
	}
	template <typename tIter>
	inline Message *new_message(uint64_t message_id, size_t sysargs, tIter sysarg_it, size_t args, tIter arg_it)
	{
		Message *msg = new_message(message_id, sysargs, args);
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
	template <typename tIter>
	inline Message *new_message(const String *name, size_t sysargs, tIter sysarg_it, size_t args, tIter arg_it)
	{
		return new_message(make_message_id(name), sysargs, sysarg_it, args, arg_it);
	}
	inline Message *new_message(const Message &other)
	{
		size_t size = sizeof(Message) + sizeof(Value)*other.total_count();
		Message *msg = value_pool.alloc<Message>(sizeof(Value)*other.total_count(), MESSAGE);
		memcpy(msg, &other, size);
		return msg;
	}

	inline void gc_scan(Message *from)
	{
		size_t count = from->m_sysarg_count + from->m_arg_count;
		for (size_t i = 0; i < count; i++)
		{
			gc_scan(from->m_data[i]);
		}
	}

	inline Value value(const Message &msg) { return make_value_ptr(MESSAGE, new_message(msg)); }
	inline Value value(const Message *msg) { return make_value_ptr(MESSAGE, msg); }
}

