#include "environment.hpp"
#include "value.hpp"
#include "istream.hpp"
#include "context.hpp"
#include "message.hpp"
#include "primitive.hpp"

#include <assert.h>

namespace Channel9
{
	Environment::Environment()
	 : m_context(NULL), m_running(false)
	{}

	const Value &Environment::special_channel(const std::string &name) const
	{
		special_map::const_iterator it = m_specials.find(name);
		if (it != m_specials.end())
			return it->second;
		else
			return Value::Nil;
	}
	void Environment::set_special_channel(const std::string &name, const Value &val)
	{
		m_specials[name] = val;
	}

	void Environment::run(RunnableContext *context)
	{
		m_context = context;
		if (!m_running)
		{
			m_running = true;
			IStream::const_iterator it = m_context->next();
			while (m_context && it != m_context->end())
			{
				size_t expected = -1;
#				define CHECK_STACK(in, out) {\
					assert(m_context->stack_count() >= (size_t)(in)); \
					if ((out) != -1) { assert((long)(expected = m_context->stack_count() - (size_t)(in) + (size_t)(out)) >= 0); } \
					DO_DEBUG printf("Stack check: before(%d), in(%d), out(%d), expected(%d)\n", (int)m_context->stack_count(), (int)(in), (int)(out), (int)expected); \
				}

				const Instruction &ins = *it;
				DO_DEBUG printf("Hi! Doing %s@%d\n", iname(ins.instruction).c_str(), (int)(it - m_context->instructions().begin()));

				switch(ins.instruction)
				{
				case NOP:
					CHECK_STACK(0, 0);
					break;
				case DEBUG:
					CHECK_STACK(0, 0);
					break;

				case POP:
					CHECK_STACK(1, 0);
					m_context->pop();
					break;
				case PUSH:
					CHECK_STACK(0, 1);
					m_context->push(ins.arg1);
					break;
				case SWAP: {
					CHECK_STACK(2, 2);
					Value tmp1 = m_context->top();
					m_context->pop();
					Value tmp2 = m_context->top();
					m_context->pop();
					m_context->push(tmp1);
					m_context->push(tmp2);
					}
					break;
				case DUP_TOP:
					CHECK_STACK(1, 2);
					m_context->push(m_context->top());
					break;

				case IS: {
					CHECK_STACK(1, 1);
					Value res = bvalue(ins.arg1 == m_context->top());
					m_context->pop();
					m_context->push(res);
					}
					break;
				case IS_EQ: {
					CHECK_STACK(2, 1);
					Value first = m_context->top();
					m_context->pop();
					Value second = m_context->top();
					m_context->pop();
					m_context->push(bvalue(first == second));
					}
					break;
				case IS_NOT: {
					CHECK_STACK(1, 1);
					Value res = bvalue(ins.arg1 != m_context->top());
					m_context->pop();
					m_context->push(res);
					}
					break;
				case IS_NOT_EQ:{
					CHECK_STACK(2, 1);
					Value first = m_context->top();
					m_context->pop();
					Value second = m_context->top();
					m_context->pop();
					m_context->push(bvalue(first != second));
					}
					break;
				case JMP:
					CHECK_STACK(0, 0);
					m_context->jump(*ins.arg1.str);
					break;
				case JMP_IF:
					CHECK_STACK(1, 0);
					if (is_truthy(m_context->top()))
						m_context->jump(*ins.arg1.str);
					m_context->pop();
					break;
				case JMP_IF_NOT:
					CHECK_STACK(1, 0);
					if (!is_truthy(m_context->top()))
						m_context->jump(*ins.arg1.str);
					m_context->pop();
					break;

				case LOCAL_CLEAN_SCOPE:
					CHECK_STACK(0, 0);
					m_context->new_scope();
					break;
				case LOCAL_LINKED_SCOPE:
					CHECK_STACK(0, 0);
					m_context->new_scope(true);
					break;
				case FRAME_GET: {
					CHECK_STACK(0, 1);
					size_t frameid = m_context->instructions().frame(*ins.arg1.str);
					m_context->push(m_context->get_framevar(frameid));
					}
					break;
				case FRAME_SET: {
					CHECK_STACK(1, 0);
					size_t frameid = m_context->instructions().frame(*ins.arg1.str);
					m_context->set_framevar(frameid, m_context->top());
					m_context->pop();
					}
					break;
				case LOCAL_GET: {
					CHECK_STACK(0, 1);
					size_t depth = ins.arg1.machine_num;
					size_t localid = m_context->instructions().local(*ins.arg2.str);
					DO_DEBUG printf("local_get %u@%u: %d\n", (unsigned)localid, (unsigned)depth, m_context->get_localvar(localid, depth).m_type);
					if (depth == 0)
						m_context->push(m_context->get_localvar(localid));
					else
						m_context->push(m_context->get_localvar(localid, depth));
					}
					break;
				case LOCAL_SET: {
					CHECK_STACK(1, 0);
					size_t depth = ins.arg1.machine_num;
					size_t localid = m_context->instructions().local(*ins.arg2.str);
					DO_DEBUG printf("local_set %u@%u: %d\n", (unsigned)localid, (unsigned)depth, m_context->top().m_type);
					if (depth == 0)
						m_context->set_localvar(localid, m_context->top());
					else
						m_context->set_localvar(localid, depth, m_context->top());
					m_context->pop();
					}
					break;

				case CHANNEL_NEW: {
					CHECK_STACK(0, 1);
					CallableContext *nctx = m_context->new_context(*ins.arg1.str);
					m_context->push(value(nctx));
					}
					break;
				case CHANNEL_SPECIAL:
					CHECK_STACK(0, 1);
					m_context->push(special_channel(*ins.arg1.str));
					break;
				case CHANNEL_SEND: {
					CHECK_STACK(3, -1);
					Value value = m_context->top(); m_context->pop();
					Value ret = m_context->top(); m_context->pop();
					Value channel = m_context->top(); m_context->pop();

					switch (channel.m_type)
					{
					case RUNNABLE_CONTEXT:
						channel.ret_ctx->send(this, value, ret);
						break;
					case CALLABLE_CONTEXT:
						channel.call_ctx->send(this, value, ret);
						break;
					default:
						printf("Built-in Channels not yet implemented.\n");
						exit(1);
					}
					}
					break;
				case CHANNEL_CALL:{
					CHECK_STACK(2, -1);
					Value val = m_context->top(); m_context->pop();
					Value channel = m_context->top(); m_context->pop();

					switch (channel.m_type)
					{
					case RUNNABLE_CONTEXT:
						channel.ret_ctx->send(this, val, value(m_context));
						break;
					case CALLABLE_CONTEXT:
						channel.call_ctx->send(this, val, value(m_context));
						break;
					case MACHINE_NUM:
						m_context->push(number_channel_simple(this, channel, *val.msg));
						m_context->push(Value::Nil);
						break;
					default:
						printf("Built-in Channel for %d not yet implemented.\n", channel.m_type);
						exit(1);
					}
					}
					break;
				case CHANNEL_RET:{
					CHECK_STACK(2, -1);
					Value value = m_context->top(); m_context->pop();
					Value channel = m_context->top(); m_context->pop();

					// TODO: Special channel for invalid returns.
					switch (channel.m_type)
					{
					case RUNNABLE_CONTEXT:
						channel.ret_ctx->send(this, value, Value::Undef);
						break;
					case CALLABLE_CONTEXT:
						channel.call_ctx->send(this, value, Value::Undef);
						break;
					default:
						printf("Built-in Channels not yet implemented.\n");
						exit(1);
					}
					}
					break;

				case MESSAGE_NEW: {
					const std::string &name = *ins.arg1.str;
					long long sysarg_count = ins.arg2.machine_num, sysarg_counter = sysarg_count - 1;
					long long arg_count = ins.arg3.machine_num, arg_counter = arg_count - 1;
					CHECK_STACK(sysarg_count + arg_count, 1);
					
					Value::vector sysargs(sysarg_count, Value::Undef);
					Value::vector args(arg_count, Value::Undef);

					while (arg_count > 0 && arg_counter >= 0)
					{
						args[arg_counter] = m_context->top();
						m_context->pop();
						--arg_counter;
					}
					while (sysarg_count > 0 && sysarg_counter >= 0)
					{
						sysargs[sysarg_counter] = m_context->top();
						m_context->pop();
						--sysarg_counter;
					}

					m_context->push(value(Message(name, sysargs, args)));
					}
					break;
				case MESSAGE_SPLAT: {
					CHECK_STACK(2, 1);
					const Value::vector &tuple = *m_context->top().tuple; m_context->pop();
					const Message &msg = *m_context->top().msg; m_context->pop();

					Message nmsg = msg;
					nmsg.add_args(tuple);
					m_context->push(value(nmsg));
					}
					break;
				case MESSAGE_ADD: {
					long long count = ins.arg1.machine_num, counter = 0;
					CHECK_STACK(1 + count, 1);
					Value::vector tuple(count, Value::Undef);
					while (count > 0 && counter < count)
					{
						tuple[count] = m_context->top();
						m_context->pop();
						++counter;
					}

					const Message &msg = *m_context->top().msg; m_context->pop();
					Message nmsg = msg;
					nmsg.add_args(tuple);
					m_context->push(value(nmsg));
					}
					break;
				case MESSAGE_COUNT:
					CHECK_STACK(1, 2);
					m_context->push(value((long long)m_context->top().msg->arg_count()));
					break;
				case MESSAGE_NAME:
					CHECK_STACK(1, 2);
					m_context->push(value(m_context->top().msg->name()));
					break;
				case MESSAGE_SYS_UNPACK: {
					long long count = ins.arg1.machine_num, pos = count - 1;
					CHECK_STACK(1, 1 + count);

					const Value::vector &tuple = m_context->top().msg->sysargs();
					long long len = tuple.size();

					while (pos >= 0)
					{
						if (pos >= len)
							m_context->push(Value::Nil);
						else
							m_context->push(tuple[pos]);
						
						pos -= 1;
					}
					}
					break;
				case MESSAGE_UNPACK: {
					long long first_count = ins.arg1.machine_num, first_counter = 0;
					long long splat = ins.arg2.machine_num;
					long long last_count = ins.arg3.machine_num, last_counter = 0;
					CHECK_STACK(1, 1 + first_count + last_count + (splat?1:0));

					const Value::vector &tuple = m_context->top().msg->args();
					long long len = tuple.size(), pos = tuple.size() - 1;
					while (last_counter < last_count && pos >= first_count)
					{
						if (pos >= len)
							m_context->push(Value::Nil);
						else
							m_context->push(tuple[pos]);

						pos -= 1;
						last_counter += 1;
					}

					if (splat != 0)
					{
						Value::vector splat_tuple;
						while (pos >= first_count)
						{
							splat_tuple.push_back(tuple[pos]);
							pos -= 1;
						}
					}

					pos = first_count - 1;

					while (first_counter < first_count && pos >= 0)
					{
						if (pos >= len)
							m_context->push(Value::Nil);
						else
							m_context->push(tuple[pos]);

						pos -= 1;
						first_counter += 1;
					}
					}
					break;

				case STRING_NEW:
					break;

				case TUPLE_NEW: {
					long long count = ins.arg1.machine_num, counter = 0;
					CHECK_STACK(count, 1);
					Value::vector tuple(count, Value::Undef);
					while (count > 0 && counter < count)
					{
						tuple[counter] = m_context->top();
						m_context->pop();
						++counter;
					}
					m_context->push(value(tuple));
					}
					break;
				case TUPLE_SPLAT: {
					CHECK_STACK(2, 1);
					const Value::vector &tuple2 = *m_context->top().tuple; m_context->pop();
					const Value::vector &tuple1 = *m_context->top().tuple; m_context->pop();
					Value::vector tuple = tuple1;
					tuple.insert(tuple.end(), tuple2.begin(), tuple2.end());
					m_context->push(value(tuple));
					}
					break;
				case TUPLE_UNPACK: {
					long long first_count = ins.arg1.machine_num, first_counter = 0;
					long long splat = ins.arg2.machine_num;
					long long last_count = ins.arg3.machine_num, last_counter = 0;
					CHECK_STACK(1, 1 + first_count + last_count + (splat?1:0));

					const Value::vector &tuple = *m_context->top().tuple;
					long long len = tuple.size(), pos = tuple.size() - 1;
					while (last_counter < last_count && pos >= first_count)
					{
						if (pos >= len)
							m_context->push(Value::Nil);
						else
							m_context->push(tuple[pos]);

						pos -= 1;
						last_counter += 1;
					}

					if (splat != 0)
					{
						Value::vector splat_tuple;
						while (pos >= first_count)
						{
							splat_tuple.push_back(tuple[pos]);
							pos -= 1;
						}
					}

					pos = first_count - 1;

					while (first_counter < first_count && pos >= 0)
					{
						if (pos >= len)
							m_context->push(Value::Nil);
						else
							m_context->push(tuple[pos]);

						pos -= 1;
						first_counter += 1;
					}
					}
					break;
				default:
					printf("Panic! Unknown instruction %d\n", ins.instruction);
					exit(1);
				}

				if ((long)expected != -1)
					assert(m_context->stack_count() == expected);

				it = m_context->next();
			}
			m_running = false;
			m_context = NULL;
		}
	}
}