#pragma once

#include "message.hpp"
#include "context.hpp"
#include "environment.hpp"

#include <sstream>
#include <math.h>

namespace Channel9
{
	inline void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Value &msg)
	{
		TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Forwarding primitive call: %s.%s from:%s to class:%s\n",
			inspect(oself).c_str(), inspect(msg).c_str(), inspect(ctx).c_str(), inspect(prim_class).c_str());
		Message *orig = ptr<Message>(msg);
		assert(orig->message_id() != MESSAGE_C9_PRIMITIVE_CALL);
		Message *fwd = new_message(make_protocol_id(PROTOCOL_C9_SYSTEM, MESSAGE_C9_PRIMITIVE_CALL), 0, 2);

		Message::iterator out = fwd->args();
		*out++ = oself;
		*out++ = value(orig);

		channel_send(cenv, prim_class, value(fwd), ctx);
	}

	inline void number_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
	{
		Message &msg = *ptr<Message>(msg_val);
		long long self = oself.machine_num;
		if (msg.arg_count() == 1 && is_number(msg.args()[0]))
		{
			long long other = msg.args()[0].machine_num;
			switch (msg.message_id())
			{
			case MESSAGE_PLUS:
				return channel_send(cenv, ctx, value(self + other), Nil);
			case MESSAGE_MINUS:
				return channel_send(cenv, ctx, value(self - other), Nil);
			case MESSAGE_MULTIPLY:
				return channel_send(cenv, ctx, value(self * other), Nil);
			case MESSAGE_DIVIDE:
				return channel_send(cenv, ctx, value(self / other), Nil);
			case MESSAGE_BITWISE_AND:
				return channel_send(cenv, ctx, value(self & other), Nil);
			case MESSAGE_BITWISE_OR:
				return channel_send(cenv, ctx, value(self | other), Nil);
			case MESSAGE_MODULUS:
				return channel_send(cenv, ctx, value(self % other), Nil);
			case MESSAGE_LT:
				return channel_send(cenv, ctx, bvalue(self < other), Nil);
			case MESSAGE_GT:
				return channel_send(cenv, ctx, bvalue(self > other), Nil);
			case MESSAGE_LT_EQUAL:
				return channel_send(cenv, ctx, bvalue(self <= other), Nil);
			case MESSAGE_GT_EQUAL:
				return channel_send(cenv, ctx, bvalue(self >= other), Nil);
			case MESSAGE_POWER:
				return channel_send(cenv, ctx, value(int64_t(pow(self, other))), Nil);
			}
		}
		switch (msg.message_id())
		{
		case MESSAGE_TO_STRING_PRIMITIVE:
			{
				std::stringstream str;
				str << oself.machine_num;
				return channel_send(cenv, ctx, value(str.str()), Nil);
			}
		case MESSAGE_TO_FLOAT_PRIMITIVE:
			return channel_send(cenv, ctx, value((double)self), Nil);
		case MESSAGE_NEGATE:
			return channel_send(cenv, ctx, value(-self), Nil);
		case MESSAGE_TO_MESSAGE_NAME:
			{
				uint64_t proto = (uint64_t)(self >> PROTOCOL_ID_SHIFT);
				uint64_t message = (uint64_t)(self & MESSAGE_ID_MASK);
				if (proto < protocol_names.size() && message < message_names.size())
				{
					if (proto > 0)
						return channel_send(cenv, ctx, value(message_names[message]), Nil);
					else
						return channel_send(cenv, ctx, value(protocol_names[proto] + ":" + message_names[message]), Nil);
				}
			}
			break;
		case MESSAGE_TO_PROTOCOL_NAME:
			{
				if ((uint64_t)self < protocol_names.size())
				{
					return channel_send(cenv, ctx, value(protocol_names[self]), Nil);
				}
			}
			break;
		case MESSAGE_HASH:
			return channel_send(cenv, ctx, value((long long)(mix_bits(uint64_t(self)) & Value::VALUE_MASK)), Nil);
		}

		Value def = cenv->special_channel("Channel9::Primitive::Number");
		forward_primitive_call(cenv, def, ctx, oself, msg_val);
	}

	inline void float_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
	{
		Message &msg = *ptr<Message>(msg_val);
		double self = float_num(oself);
		if (msg.arg_count() == 1 && is(msg.args()[0], FLOAT_NUM))
		{
			double other = float_num(msg.args()[0]);
			switch (msg.message_id())
			{
			case MESSAGE_PLUS:
				return channel_send(cenv, ctx, value(self + other), Nil);
			case MESSAGE_MINUS:
				return channel_send(cenv, ctx, value(self - other), Nil);
			case MESSAGE_MULTIPLY:
				return channel_send(cenv, ctx, value(self * other), Nil);
			case MESSAGE_DIVIDE:
				return channel_send(cenv, ctx, value(self / other), Nil);
			case MESSAGE_LT:
				return channel_send(cenv, ctx, bvalue(self < other), Nil);
			case MESSAGE_GT:
				return channel_send(cenv, ctx, bvalue(self > other), Nil);
			case MESSAGE_LT_EQUAL:
				return channel_send(cenv, ctx, bvalue(self <= other), Nil);
			case MESSAGE_GT_EQUAL:
				return channel_send(cenv, ctx, bvalue(self >= other), Nil);
			case MESSAGE_POWER:
				return channel_send(cenv, ctx, value(int64_t(pow(self, other))), Nil);			
			}
		}
		switch (msg.message_id())
		{
		case MESSAGE_TO_STRING_PRIMITIVE:
			{
				std::stringstream str;
				str << float_num(oself);
				return channel_send(cenv, ctx, value(str.str()), Nil);
			}
		case MESSAGE_TO_NUM_PRIMITIVE:
			return channel_send(cenv, ctx, value((int64_t)self), Nil);
		case MESSAGE_NEGATE:
			return channel_send(cenv, ctx, value(-self), Nil);
		}
		Value def = cenv->special_channel("Channel9::Primitive::Float");
		forward_primitive_call(cenv, def, ctx, oself, msg_val);
	}

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
		case MESSAGE_MATCH:
			if (msg->arg_count() == 1 && is(msg->args()[0], STRING))
			{
				String *other = ptr<String>(msg->args()[0]);
				ptrdiff_t diff;
				if (self->m_count > other->m_count)
					diff = std::mismatch(other->begin(), other->end(), self->begin()).first - other->begin();
				else
					diff = std::mismatch(self->begin(), self->end(), other->begin()).first - self->begin();
				return channel_send(cenv, ctx, value(int64_t(diff)), Nil);
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

	inline void tuple_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
	{
		Message &msg = *ptr<Message>(msg_val);
		switch (msg.message_id())
		{
		case MESSAGE_AT:
			if (msg.arg_count() == 1 && is_number(msg.args()[0]))
			{
				const Tuple &self = *ptr<Tuple>(oself);
				ssize_t idx = (ssize_t)msg.args()[0].machine_num;
				if (idx >= 0 && (size_t)idx < self.size())
				{
					return channel_send(cenv, ctx, self[idx], Nil);
				} else if (idx < 0 && (ssize_t)self.size() >= -idx) {
					return channel_send(cenv, ctx, self[self.size() - idx], Nil);
				}
			}
			break;
		case MESSAGE_LENGTH:
			return channel_send(cenv, ctx, value((long long)ptr<Tuple>(oself)->size()), Nil);
		case MESSAGE_PLUS:
			return channel_send(cenv, ctx, value(join_tuple(ptr<Tuple>(oself), ptr<Tuple>(msg.args()[0]))), Nil);
		case MESSAGE_PUSH:
			return channel_send(cenv, ctx, value(join_tuple(ptr<Tuple>(oself), msg.args()[0])), Nil);
		case MESSAGE_POP:
			{
				Tuple *tuple = ptr<Tuple>(oself);
				if (tuple->m_count > 0)
					return channel_send(cenv, ctx, value(sub_tuple(tuple, 0, tuple->m_count - 1)), Nil);
			}
			break;
		case MESSAGE_FRONT_PUSH:
			return channel_send(cenv, ctx, value(join_tuple(msg.args()[0], ptr<Tuple>(oself))), Nil);
		case MESSAGE_FRONT_POP:
			{
				Tuple *tuple = ptr<Tuple>(oself);
				if (tuple->m_count > 0)
					return channel_send(cenv, ctx, value(sub_tuple(tuple, 1, tuple->m_count - 1)), Nil);
			}
			break;
		case MESSAGE_REPLACE:
			{
				Tuple *tuple = ptr<Tuple>(oself);
				long long idx = msg.args()[0].machine_num;
				Value val = msg.args()[1];
				if (idx >= 0)
					return channel_send(cenv, ctx, value(replace_tuple(tuple, (size_t)idx, val)), Nil);
			}
			break;
		case MESSAGE_FIRST:
			{
				Tuple *tuple = ptr<Tuple>(oself);
				if (tuple->m_count > 0)
					return channel_send(cenv, ctx, tuple->m_data[0], Nil);
			}
			break;
		case MESSAGE_LAST:
			{
				Tuple *tuple = ptr<Tuple>(oself);
				if (tuple->m_count > 0)
					return channel_send(cenv, ctx, tuple->m_data[tuple->m_count-1], Nil);
			}
			break;
		}

		Value def = cenv->special_channel("Channel9::Primitive::Tuple");
		forward_primitive_call(cenv, def, ctx, oself, msg_val);
	}

	inline void message_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
	{
		Message &msg = *ptr<Message>(msg_val);
		Message &self = *ptr<Message>(oself);

		switch (msg.message_id())
		{
		case MESSAGE_NAME:
			return channel_send(cenv, ctx, value(self.name()), Nil);
		}

		Value def = cenv->special_channel("Channel9::Primitive::Message");
		forward_primitive_call(cenv, def, ctx, oself, msg_val);
	}
}

