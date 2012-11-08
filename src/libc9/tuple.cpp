#include "c9/tuple.hpp"
#include "c9/context.hpp"

namespace Channel9
{
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
					return channel_send(cenv, ctx, self[self.size() + idx], Nil);
				}
			}
			break;
		case MESSAGE_SUBARY:
			if (msg.arg_count() == 2 && is_number(msg.args()[0]) && is_number(msg.args()[1]))
			{
				const Tuple *self = ptr<Tuple>(oself);
				size_t first = (size_t)msg.args()[0].machine_num,
					last = (size_t)msg.args()[1].machine_num;
				if (first >= 0 && last >= 0 && first <= last && first <= self->size() && last <= self->size())
				{
					return channel_send(cenv, ctx, value(sub_tuple(self, first, last - first)), Nil);
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
		case MESSAGE_DELETE:
			{
				Tuple *tuple = ptr<Tuple>(oself);
				long long idx = msg.args()[0].machine_num;
				if (idx >= 0 && size_t(idx) < tuple->m_count)
					return channel_send(cenv, ctx, value(remove_tuple(tuple, (size_t)idx)), Nil);
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

	INIT_SEND_FUNC(TUPLE, &tuple_channel_simple);
}
