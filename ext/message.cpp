#include "message.hpp"

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
		InitializeMessages()
		{
			uint64_t proto_counter = PROTOCOL_DEFAULT;
			uint64_t message_counter = MESSAGE_NOP;

#			define MAKE_PROTOCOL(name) \
				protocol_ids[name] = proto_counter++; \
				protocol_names.push_back(name)
#			define MAKE_MESSAGE(name) \
				message_ids[name] = message_counter++; \
				message_names.push_back(name)

			MAKE_PROTOCOL("");
			MAKE_PROTOCOL("c9");

			MAKE_MESSAGE("nop");
			MAKE_MESSAGE("+");
			MAKE_MESSAGE("-");
			MAKE_MESSAGE("*");
			MAKE_MESSAGE("/");
			MAKE_MESSAGE("&");
			MAKE_MESSAGE("|");
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
			MAKE_MESSAGE("hash");

			MAKE_MESSAGE("split");
			MAKE_MESSAGE("substr");
			MAKE_MESSAGE("match");
			MAKE_MESSAGE("length");

			MAKE_MESSAGE("at");
			MAKE_MESSAGE("push");
			MAKE_MESSAGE("pop");
			MAKE_MESSAGE("front_push");
			MAKE_MESSAGE("front_pop");
			MAKE_MESSAGE("replace");
			MAKE_MESSAGE("first");
			MAKE_MESSAGE("last");

			MAKE_MESSAGE("name");

			MAKE_MESSAGE("__c9_primitive_call__");
		}	
	} init_msgs;

	template <typename tIt>
	uint64_t make_message_id(tIt begin, tIt end)
	{
		uint64_t protocol = PROTOCOL_DEFAULT, message = MESSAGE_NOP;
		message_id_map::iterator name_finder;

		tIt split = std::find(begin, end, ':');
		if (split != end)
		{
			std::string protoname(begin, split);
			name_finder = protocol_ids.find(protoname);
			if (name_finder != protocol_ids.end()) {
				protocol = name_finder->second;
			} else {
				protocol = protocol_names.size();
				protocol_names.push_back(protoname);
			}
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
		}
		return (protocol << PROTOCOL_ID_SHIFT) | message;
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
}