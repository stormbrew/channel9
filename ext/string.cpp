#include "string.hpp"
#include "context.hpp"

namespace Channel9
{
	inline void string_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
	{
		Message *msg = ptr<Message>(msg_val);
		String *self = ptr<String>(oself);

		switch (msg->message_id())
		{
		case MESSAGE_PLUS:
			if (msg->arg_count() == 1 && is(msg->args()[0], STRING))
			{
				String *other = ptr<String>(msg->args()[0]);
				String *ret = new_string(self->m_count + other->m_count);
				std::copy(self->begin(), self->end(), ret->m_data);
				std::copy(other->begin(), other->end(), ret->m_data + self->m_count);
				return channel_send(cenv, ctx, value(ret), Nil);
			}
			break;
		case MESSAGE_TO_NUM_PRIMITIVE:
			{
				std::stringstream str(self->c_str());
				long long num = 0;
				str >> num;
				return channel_send(cenv, ctx, value(num), Nil);
			}
		case MESSAGE_TO_CHR:
			{
				if (self->m_count == 1)
					return channel_send(cenv, ctx, value((long long)self->m_data[0]), Nil);
			}
			break;
		case MESSAGE_TO_MESSAGE_ID:
			if (self->size() > 0)
			{
				return channel_send(cenv, ctx, value((long long)make_message_id(self)), Nil);
			}
			break;
		case MESSAGE_TO_PROTOCOL_ID:
			if (self->size() > 0)
			{
				return channel_send(cenv, ctx, value((long long)make_protocol_id(self)), Nil);
			}
			break;
		case MESSAGE_SPLIT:
			if (msg->arg_count() == 1 && is(msg->args()[0], STRING))
			{
				String *by = ptr<String>(msg->args()[0]);
				return channel_send(cenv, ctx, value(split_string(self, by)), Nil);
			}
			break;
		case MESSAGE_SUBSTR:
			if (msg->arg_count() == 2 && is(msg->args()[0], POSITIVE_NUMBER) && is(msg->args()[1], POSITIVE_NUMBER))
			{
				size_t first = msg->args()[0].machine_num, last = msg->args()[1].machine_num;
				if (first < self->m_count && last < self->m_count && last >= first)
					return channel_send(cenv, ctx, value(self->substr(first, last - first + 1)), Nil);
			}
			break;
		case MESSAGE_COMPARE:
			if (msg->arg_count() == 1 && is(msg->args()[0], STRING))
			{
				String *other = ptr<String>(msg->args()[0]);
				return channel_send(cenv, ctx, value(int64_t(strncmp(self->c_str(), other->c_str(), 
					std::min(self->m_count, other->m_count)))), Nil);
			}
			break;
		case MESSAGE_HASH:
			{
				// djb2 algorithm
				unsigned long hash = 5381;

				for (String::iterator it = self->begin(); it != self->end(); it++)
				{
					hash = ((hash << 5) + hash) ^ *it;
				}
				hash &= Value::VALUE_MASK; // don't want negative hashes.

				return channel_send(cenv, ctx, value((long long)hash), Nil);
			}
		case MESSAGE_LENGTH:
			return channel_send(cenv, ctx, value((long long)self->length()), Nil);
		case MESSAGE_EQUAL:
			if (msg->arg_count() == 1 && is(msg->args()[0], STRING))
			{
				String *other = ptr<String>(msg->args()[0]);
				return channel_send(cenv, ctx, bvalue(*self == *other), Nil);
			}
			break;
		}
		Value def = cenv->special_channel("Channel9::Primitive::String");
		forward_primitive_call(cenv, def, ctx, oself, msg_val);
	}
	INIT_SEND_FUNC(STRING, &string_channel_simple);
}