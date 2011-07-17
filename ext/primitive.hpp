#pragma once

#include "message.hpp"
#include "context.hpp"
#include "environment.hpp"

#include <sstream>

namespace Channel9
{
	inline void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Message &msg)
	{
		static String *prim_call = new_string("__c9_primitive_call__");
		DO_TRACE printf("Forwarding primitive call: %s.%s from:%s to class:%s\n", 
			inspect(oself).c_str(), inspect(value(msg)).c_str(), inspect(ctx).c_str(), inspect(prim_class).c_str());
		Message *fwd = new_message(prim_call, msg.sysarg_count(), msg.arg_count() + 2);
		std::copy(msg.sysargs(), msg.sysargs_end(), fwd->sysargs());

		Message::iterator out = fwd->args();
		*out++ = value(msg.name());
		*out++ = oself;
		std::copy(msg.args(), msg.args_end(), out);

		channel_send(cenv, prim_class, value(fwd), ctx);	
	}

	inline void number_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg)
	{
		const String &name = *msg.name();
		switch (name.length())
		{
		case 1:
			if (msg.arg_count() == 1 && msg.args()[0].m_type == MACHINE_NUM)
			{
				long long other = msg.args()[0].machine_num, self = oself.machine_num;
				switch (name[0])
				{
				case '+':
					return channel_send(cenv, ctx, value(self + other), Value::Nil);
				case '-':
					return channel_send(cenv, ctx, value(self - other), Value::Nil);
				case '*':
					return channel_send(cenv, ctx, value(self * other), Value::Nil);
				case '/':
					return channel_send(cenv, ctx, value(self / other), Value::Nil);
				case '%':
					return channel_send(cenv, ctx, value(self % other), Value::Nil);
				case '<':
					return channel_send(cenv, ctx, bvalue(self < other), Value::Nil);
				case '>':
					return channel_send(cenv, ctx, bvalue(self > other), Value::Nil);
				}
			}
			break;
		case 2:
			if (msg.arg_count() == 1 && msg.args()[0].m_type == MACHINE_NUM && name[1] == '=')
			{
				long long other = msg.args()[0].machine_num, self = oself.machine_num;
				switch (name[0])
				{
				case '<':
					return channel_send(cenv, ctx, bvalue(self <= other), Value::Nil);
				case '>':
					return channel_send(cenv, ctx, bvalue(self >= other), Value::Nil);
				}
			}
			break;
		default:
			if (name == "to_string_primitive")
			{
				std::stringstream str;
				str << oself.machine_num;
				return channel_send(cenv, ctx, value(str.str()), Value::Nil);
			}
		}
		Value def = cenv->special_channel("Channel9::Primitive::Number");
		forward_primitive_call(cenv, def, ctx, oself, msg);
	}

	inline void string_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg)
	{
		const String &name = *msg.name();
		if (name.length() == 1)
		{
			if (msg.arg_count() == 1 && msg.args()[0].m_type == STRING)
			{
				const String *other = msg.args()[0].str, *self = oself.str;
				switch (name[0])
				{
				case '+':
					return channel_send(cenv, ctx, value(join_string(self, other)), Value::Nil);
				}
			}
		} else if (name == "length") {
			return channel_send(cenv, ctx, value((long long)oself.str->length()), Value::Nil);
		}
		Value def = cenv->special_channel("Channel9::Primitive::String");
		forward_primitive_call(cenv, def, ctx, oself, msg);
	}

	inline void tuple_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg)
	{
		const String &name = *msg.name();
		if (name == "at") {
			if (msg.arg_count() == 1 && msg.args()[0].m_type == MACHINE_NUM)
			{
				const Tuple &self = *oself.tuple;
				ssize_t idx = (ssize_t)msg.args()[0].machine_num;
				if (idx >= 0 && (size_t)idx < self.size())
				{
					return channel_send(cenv, ctx, self[idx], Value::Nil);
				} else if (idx < 0 && (ssize_t)self.size() >= -idx) {
					return channel_send(cenv, ctx, self[self.size() - idx], Value::Nil);
				}
			}
		} else if (name == "length") {
			return channel_send(cenv, ctx, value((long long)oself.tuple->size()), Value::Nil);
		}

		Value def = cenv->special_channel("Channel9::Primitive::Tuple");
		forward_primitive_call(cenv, def, ctx, oself, msg);
	}
}