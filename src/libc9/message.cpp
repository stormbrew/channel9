#include "c9/message.hpp"
#include "c9/context.hpp"

#include <algorithm>

namespace Channel9
{
	message_id_map protocol_ids;
	message_id_map message_ids;
	message_name_list message_names;
	message_name_list protocol_names;

	static class InitializeMessages
	{
	public:
		InitializeMessages();
	} init_msgs;

	InitializeMessages::InitializeMessages()
	{
		uint64_t proto_counter = PROTOCOL_DEFAULT;
		uint64_t message_counter = MESSAGE_NOP;

#			define MAKE_PROTOCOL(name) \
			protocol_ids[name] = proto_counter++; \
			protocol_names.push_back(name);
#			define MAKE_MESSAGE(name) \
			message_ids[name] = message_counter++; \
			message_names.push_back(name);

		MAKE_PROTOCOL("");
		MAKE_PROTOCOL("c9");

		MAKE_MESSAGE("nop");
		MAKE_MESSAGE("+");
		MAKE_MESSAGE("-");
		MAKE_MESSAGE("*");
		MAKE_MESSAGE("/");
		MAKE_MESSAGE("&");
		MAKE_MESSAGE("|");
		MAKE_MESSAGE("^");
		MAKE_MESSAGE("<<");
		MAKE_MESSAGE(">>");
		MAKE_MESSAGE("%");
		MAKE_MESSAGE("<");
		MAKE_MESSAGE(">");
		MAKE_MESSAGE("==");
		MAKE_MESSAGE("<=");
		MAKE_MESSAGE(">=");
		MAKE_MESSAGE("**");
		MAKE_MESSAGE("negate");

		MAKE_MESSAGE("to_string_primitive");
		MAKE_MESSAGE("to_num_primitive");
		MAKE_MESSAGE("to_float_primitive");
		MAKE_MESSAGE("to_tuple_primitive");
		MAKE_MESSAGE("to_chr");
		MAKE_MESSAGE("to_message_id");
		MAKE_MESSAGE("to_protocol_id");
		MAKE_MESSAGE("to_message_name");
		MAKE_MESSAGE("to_protocol_name");

		MAKE_MESSAGE("hash");

		MAKE_MESSAGE("split");
		MAKE_MESSAGE("substr");
		MAKE_MESSAGE("compare");
		MAKE_MESSAGE("length");

		MAKE_MESSAGE("at");
		MAKE_MESSAGE("subary");
		MAKE_MESSAGE("push");
		MAKE_MESSAGE("pop");
		MAKE_MESSAGE("front_push");
		MAKE_MESSAGE("front_pop");
		MAKE_MESSAGE("replace");
		MAKE_MESSAGE("delete");
		MAKE_MESSAGE("first");
		MAKE_MESSAGE("last");

		MAKE_MESSAGE("name");
		MAKE_MESSAGE("unforward");

		MAKE_MESSAGE("primitive_call");
	}

	template <typename tIt>
	uint64_t make_protocol_id(tIt begin, tIt end)
	{
		uint64_t protocol = PROTOCOL_DEFAULT;
		std::string protoname(begin, end);
		message_id_map::iterator name_finder = protocol_ids.find(protoname);

		if (name_finder != protocol_ids.end()) {
			protocol = name_finder->second;
		} else {
			protocol = protocol_names.size();
			protocol_names.push_back(protoname);
			protocol_ids[protoname] = protocol;
		}
		return protocol;
	}

	template <typename tIt>
	uint64_t make_message_id(tIt begin, tIt end)
	{
		uint64_t protocol = PROTOCOL_DEFAULT, message = MESSAGE_NOP;
		message_id_map::iterator name_finder;

		tIt split = std::find(begin, end, ':');
		if (split != end)
		{
			protocol = make_protocol_id(begin, split);
			begin = split + 1;
		}
		assert(protocol < 0xff);

		std::string messagename(begin, end);
		name_finder = message_ids.find(messagename);
		if (name_finder != message_ids.end())
		{
			message = name_finder->second;
		} else {
			message = message_names.size();
			message_names.push_back(messagename);
			message_ids[messagename] = message;
		}
		TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Message ID: %s -> %"PRIu64"\n", messagename.c_str(), message);
		return (protocol << PROTOCOL_ID_SHIFT) | message;
	}

	uint64_t make_protocol_id(const std::string &name)
	{
		return make_protocol_id(name.begin(), name.end());
	}
	uint64_t make_protocol_id(const String *name)
	{
		return make_protocol_id(name->begin(), name->end());
	}

	uint64_t make_message_id(const std::string &name)
	{
		return make_message_id(name.begin(), name.end());
	}
	uint64_t make_message_id(const String *name)
	{
		return make_message_id(name->begin(), name->end());
	}

	const std::string &Message::protocol() const
	{
		uint64_t proto = protocol_id();
		assert(proto < protocol_names.size());
		return protocol_names[proto];
	}
	const std::string &Message::simple_name() const
	{
		uint64_t nameid = message_id();
		assert(nameid < message_names.size());
		return message_names[nameid];
	}
	std::string Message::name() const
	{
		if (protocol_id() > 0)
			return protocol() + ":" + simple_name();
		else
			return simple_name();
	}

	void message_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
	{
		Message &msg = *ptr<Message>(msg_val);
		Message &self = *ptr<Message>(oself);

		switch (msg.message_id())
		{
		case MESSAGE_NAME:
			return channel_send(cenv, ctx, value(self.name()), Nil);
		case MESSAGE_UNFORWARD:
			if (self.arg_count() > 0 && is(self.args()[0], STRING))
			{
				String *name = ptr<String>(self.args()[0]);
				Message *nmsg = new_message(make_message_id(name), self.sysarg_count(), self.arg_count() - 1);
				std::copy(self.sysargs(), self.sysargs_end(), nmsg->sysargs());
				std::copy(self.args() + 1, self.args_end(), nmsg->args());
				return channel_send(cenv, ctx, value(nmsg), Nil);
			}
			break;
		}

		Value def = cenv->special_channel("Channel9::Primitive::Message");
		forward_primitive_call(cenv, def, ctx, oself, msg_val);
	}
	INIT_SEND_FUNC(MESSAGE, &message_channel_simple);

	static void scan_message(Message *from)
	{
		size_t count = from->m_sysarg_count + from->m_arg_count;
		for (size_t i = 0; i < count; i++)
		{
			gc_scan(from->m_data[i]);
		}
	}
	INIT_SCAN_FUNC(MESSAGE, &scan_message);

}
