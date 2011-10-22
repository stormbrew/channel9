#include "primitive.hpp"
#include "context.hpp"

namespace Channel9
{
	void number_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
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
	INIT_SEND_FUNC(POSITIVE_NUMBER, &number_channel_simple);
	INIT_SEND_FUNC(NEGATIVE_NUMBER, &number_channel_simple);

	void float_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg_val)
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
	INIT_SEND_FUNC(FLOAT_NUM, &float_channel_simple);

	void nil_channel_simple(Environment *env, const Value &ret, const Value &oself, const Value &val)
	{
		const Value &def = env->special_channel("Channel9::Primitive::NilC");
		return forward_primitive_call(env, def, ret, oself, val);
	}
	INIT_SEND_FUNC(NIL, &nil_channel_simple);
	void undef_channel_simple(Environment *env, const Value &ret, const Value &oself, const Value &val)
	{
		const Value &def = env->special_channel("Channel9::Primitive::UndefC");
		return forward_primitive_call(env, def, ret, oself, val);
	}
	INIT_SEND_FUNC(UNDEF, &undef_channel_simple);
	void false_channel_simple(Environment *env, const Value &ret, const Value &oself, const Value &val)
	{
		const Value &def = env->special_channel("Channel9::Primitive::FalseC");
		return forward_primitive_call(env, def, ret, oself, val);
	}
	INIT_SEND_FUNC(BFALSE, &false_channel_simple);
	void true_channel_simple(Environment *env, const Value &ret, const Value &oself, const Value &val)
	{
		const Value &def = env->special_channel("Channel9::Primitive::TrueC");
		return forward_primitive_call(env, def, ret, oself, val);
	}
	INIT_SEND_FUNC(BTRUE, &true_channel_simple);
}