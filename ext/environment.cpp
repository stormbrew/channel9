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
			return Nil;
	}

	const Value &Environment::special_channel(const String &name) const
	{
		special_map::const_iterator it = m_specials.find(name.c_str());
		if (it != m_specials.end())
			return it->second;
		else
			return Nil;
	}

	void Environment::set_special_channel(const std::string &name, const Value &val)
	{
		DO_TRACE printf("set_special_channel(%s, %s)\n", name.c_str(), inspect(val).c_str());
		m_specials[name] = val;
	}

	void Environment::run(RunnableContext *context)
	{
		m_context = context;
		if (!context)
		{
			m_running = false;
		} else if (!m_running)
		{
			DO_TRACE printf("Entering running state with %p\n", context);
			m_running = true;
			const Instruction *it = m_context->next();
			while (m_context && it != m_context->end())
			{
				size_t output = -1;
				size_t expected = -1;
#				define CHECK_STACK(in, out) DO_DEBUG {\
					assert(m_context->stack_count() >= (size_t)(in)); \
					if ((out) != -1) { assert((long)(expected = m_context->stack_count() - (size_t)(in) + (size_t)(out)) >= 0); } \
					DO_TRACE printf("Stack info: before(%d), in(%d), out(%d), expected(%d)\n", (int)m_context->stack_count(), (int)(in), (int)(out), (int)expected); \
					output = (out); \
				}

				const Instruction &ins = *it;
				DO_TRACE {
					printf("Instruction: %s@%d\n", 
						inspect(ins).c_str(),
						(int)(it - &*m_context->instructions().begin())
						);
					printf("Stack: %d deep", (int)m_context->stack_count());
					const Value *it;
					for (it = m_context->stack_begin(); it != m_context->stack_pos(); it++)
					{
						printf("\n   %s", inspect(*it).c_str());
					}
					printf(" <--\n");
				}

				switch(ins.instruction)
				{
				case NOP:
					CHECK_STACK(0, 0);
					break;
				case DEBUGGER:
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
					m_context->jump(ins.arg3.machine_num);
					break;
				case JMP_IF:
					CHECK_STACK(1, 0);
					if (is_truthy(m_context->top()))
						m_context->jump(ins.arg3.machine_num);
					m_context->pop();
					break;
				case JMP_IF_NOT:
					CHECK_STACK(1, 0);
					if (!is_truthy(m_context->top()))
						m_context->jump(ins.arg3.machine_num);
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
					size_t frameid = (size_t)ins.arg3.machine_num;
					DO_TRACE printf("Getting frame var %s (%d)\n", ptr<String>(ins.arg1)->c_str(), (int)frameid);
					m_context->push(m_context->get_framevar(frameid));
					}
					break;
				case FRAME_SET: {
					CHECK_STACK(1, 0);
					size_t frameid = (size_t)ins.arg3.machine_num;
					DO_TRACE printf("Setting frame var %s (%d) to: %s\n", ptr<String>(ins.arg1)->c_str(), (int)frameid, inspect(m_context->top()).c_str());
					m_context->set_framevar(frameid, m_context->top());
					m_context->pop();
					}
					break;
				case LOCAL_GET: {
					CHECK_STACK(0, 1);
					size_t depth = ins.arg1.machine_num;
					size_t localid = (size_t)ins.arg3.machine_num;
					DO_TRACE printf("local_get %u@%u: %llu\n", (unsigned)localid, (unsigned)depth, type(m_context->get_localvar(localid, depth)));
					if (depth == 0)
						m_context->push(m_context->get_localvar(localid));
					else
						m_context->push(m_context->get_localvar(localid, depth));
					}
					break;
				case LOCAL_SET: {
					CHECK_STACK(1, 0);
					size_t depth = ins.arg1.machine_num;
					size_t localid = (size_t)ins.arg3.machine_num;
					DO_TRACE printf("local_set %u@%u: %llu\n", (unsigned)localid, (unsigned)depth, type(m_context->top()));
					if (depth == 0)
						m_context->set_localvar(localid, m_context->top());
					else
						m_context->set_localvar(localid, depth, m_context->top());
					m_context->pop();
					}
					break;

				case CHANNEL_NEW: {
					CHECK_STACK(0, 1);
					RunnableContext *nctx = new_context(*m_context);
					nctx->jump(ins.arg3.machine_num);
					m_context->push(value(nctx));
					}
					break;
				case CHANNEL_SPECIAL:
					CHECK_STACK(0, 1);
					m_context->push(special_channel(*ptr<String>(ins.arg1)));
					break;
				case CHANNEL_SEND: {
					CHECK_STACK(3, -1);
					Value val = m_context->top(); m_context->pop();
					Value ret = m_context->top(); m_context->pop();
					Value channel = m_context->top(); m_context->pop();

					channel_send(this, channel, val, ret);
					}
					break;
				case CHANNEL_CALL:{
					CHECK_STACK(2, -1);
					Value val = m_context->top(); m_context->pop();
					Value channel = m_context->top(); m_context->pop();

					channel_send(this, channel, val, value(m_context));
					}
					break;
				case CHANNEL_RET:{
					CHECK_STACK(2, -1);
					Value val = m_context->top(); m_context->pop();
					Value channel = m_context->top(); m_context->pop();

					// TODO: Special channel for invalid returns.
					channel_send(this, channel, val, value(m_context));
					}
					break;

				case MESSAGE_NEW: {
					String *name = ptr<String>(ins.arg1);
					long long sysarg_count = ins.arg2.machine_num, sysarg_counter = sysarg_count - 1;
					long long arg_count = ins.arg3.machine_num, arg_counter = arg_count - 1;
					CHECK_STACK(sysarg_count + arg_count, 1);
					
					Message *msg = new_message(name, sysarg_count, arg_count);

					while (arg_count > 0 && arg_counter >= 0)
					{
						msg->args()[arg_counter] = m_context->top();
						m_context->pop();
						--arg_counter;
					}
					while (sysarg_count > 0 && sysarg_counter >= 0)
					{
						msg->sysargs()[sysarg_counter] = m_context->top();
						m_context->pop();
						--sysarg_counter;
					}

					m_context->push(value(msg));
					}
					break;
				case MESSAGE_SPLAT: {
					CHECK_STACK(2, 1);
					const Tuple &tuple = *ptr<Tuple>(m_context->top()); m_context->pop();
					const Message &msg = *ptr<Message>(m_context->top()); m_context->pop();

					Message *nmsg = new_message(msg.m_name, msg.sysarg_count(), msg.arg_count() + tuple.size());
					std::copy(msg.sysargs(), msg.sysargs_end(), nmsg->sysargs());
					std::copy(msg.args(), msg.args_end(), nmsg->args());
					std::copy(tuple.begin(), tuple.end(), nmsg->args() + msg.arg_count());
					m_context->push(value(nmsg));
					}
					break;
				case MESSAGE_ADD: {
					long long count = ins.arg1.machine_num, counter = 0;
					CHECK_STACK(1 + count, 1);
					const Message &msg = *ptr<Message>(*(m_context->stack_pos() - 1 - count));

					Message *nmsg = new_message(msg.m_name, msg.sysarg_count(), msg.arg_count() + count);
					std::copy(msg.sysargs(), msg.sysargs_end(), nmsg->sysargs());
					std::copy(msg.args(), msg.args_end(), nmsg->args());

					Message::iterator out = nmsg->args() + msg.arg_count();
					while (count > 0 && counter < count)
					{
						*out++ = m_context->top();
						m_context->pop();
						++counter;
					}
					m_context->pop(); // this is the message we pulled directly off the stack before.

					m_context->push(value(nmsg));
					}
					break;
				case MESSAGE_COUNT:
					CHECK_STACK(1, 2);
					m_context->push(value((long long)ptr<Message>(m_context->top())->arg_count()));
					break;
				case MESSAGE_NAME:
					CHECK_STACK(1, 2);
					m_context->push(value(ptr<Message>(m_context->top())->name()));
					break;
				case MESSAGE_SYS_UNPACK: {
					long long count = ins.arg1.machine_num, pos = count - 1;
					CHECK_STACK(1, 1 + count);

					const Message *msg = ptr<Message>(m_context->top());
					Message::const_iterator args = msg->sysargs();
					long long len = (long long)msg->sysarg_count();

					while (pos >= 0)
					{
						if (pos >= len)
							m_context->push(Undef);
						else
							m_context->push(args[pos]);
						
						pos -= 1;
					}
					}
					break;
				case MESSAGE_UNPACK: {
					long long first_count = ins.arg1.machine_num, first_counter = 0;
					long long splat = ins.arg2.machine_num;
					long long last_count = ins.arg3.machine_num, last_counter = 0;
					CHECK_STACK(1, 1 + first_count + last_count + (splat?1:0));

					const Message *msg = ptr<Message>(m_context->top());
					Message::const_iterator args = msg->args();
					long long len = (long long)msg->arg_count(), pos = len - 1;

					while (last_counter < last_count && pos >= first_count)
					{
						if (pos >= len)
							m_context->push(Nil);
						else
							m_context->push(args[pos]);

						pos -= 1;
						last_counter += 1;
					}

					if (splat != 0)
					{
						if (pos >= first_count)
						{
							Tuple *splat_tuple = new_tuple(pos - first_count + 1);
							Tuple::iterator out = splat_tuple->end();
							while (pos >= first_count)
							{
								--out;
								*out = args[pos];
								pos -= 1;
							}
							m_context->push(value(splat_tuple));
						} else {
							m_context->push(value(new_tuple(0)));
						}
					}

					pos = first_count - 1;

					while (first_counter < first_count && pos >= 0)
					{
						if (pos >= len)
							m_context->push(Nil);
						else
							m_context->push(args[pos]);

						pos -= 1;
						first_counter += 1;
					}
					}
					break;

				case STRING_COERCE: {
					String *coerce = ptr<String>(ins.arg1);
					const Value &val = m_context->top();
					if (is(val, STRING))
					{
						CHECK_STACK(1, 2);
						// push a nil like we called the method.
						m_context->push(Nil);
					} else {
						CHECK_STACK(1, -1);
						m_context->pop();
						channel_send(this, val, value(new_message(coerce)), value(m_context));
					}
					}
					break;

				case STRING_NEW: {
					long long count = ins.arg1.machine_num, counter = 0;
					CHECK_STACK(count, 1);
					const Value *sp = m_context->stack_pos() - 1;
					size_t len = 0;
					while (count > 0 && counter < count)
					{
						assert(is(sp[-counter], STRING));
						len += ptr<String>(sp[-counter])->length();
						++counter;
					}
					String *res = new_string(len);
					String::iterator out = res->begin();
					counter = 0;
					while (count > 0 && counter < count)
					{
						const String *val = ptr<String>(m_context->top());
						out = std::copy(val->begin(), val->end(), out);
						m_context->pop();
						++counter;
					}
					m_context->push(value(res));
					}
					break;

				case TUPLE_NEW: {
					long long count = ins.arg1.machine_num, counter = 0;
					CHECK_STACK(count, 1);
					Tuple *tuple = new_tuple(count);
					Tuple::iterator out = tuple->begin();
					while (count > 0 && counter < count)
					{
						*out++ = m_context->top();
						m_context->pop();
						++counter;
					}
					m_context->push(value(tuple));
					}
					break;
				case TUPLE_SPLAT: {
					CHECK_STACK(2, 1);
					const Tuple *tuple2 = ptr<Tuple>(m_context->top()); m_context->pop();
					const Tuple *tuple1 = ptr<Tuple>(m_context->top()); m_context->pop();
					Tuple *tuple = join_tuple(tuple1, tuple2);
					m_context->push(value(tuple));
					}
					break;
				case TUPLE_UNPACK: {
					long long first_count = ins.arg1.machine_num, first_counter = 0;
					long long splat = ins.arg2.machine_num;
					long long last_count = ins.arg3.machine_num, last_counter = 0;
					CHECK_STACK(1, 1 + first_count + last_count + (splat?1:0));

					const Tuple &tuple = *ptr<Tuple>(m_context->top());
					long long len = tuple.size(), pos = tuple.size() - 1;
					while (last_counter < last_count && pos >= first_count)
					{
						if (pos >= len)
							m_context->push(Nil);
						else
							m_context->push(tuple[pos]);

						pos -= 1;
						last_counter += 1;
					}

					if (splat != 0)
					{
						if (pos >= first_count)
						{
							Tuple *splat_tuple = new_tuple(pos - first_count + 1);
							Tuple::iterator out = splat_tuple->end();
							while (pos >= first_count)
							{
								--out;
								*out = tuple[pos];
								pos -= 1;
							}
							m_context->push(value(splat_tuple));
						} else {
							m_context->push(value(new_tuple(0)));
						}
					}

					pos = first_count - 1;

					while (first_counter < first_count && pos >= 0)
					{
						if (pos >= len)
							m_context->push(Nil);
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

				DO_TRACE {
					printf("Output: %d", (int)output);
					if ((long)output > 0)
					{
						const Value *it = std::max(
							m_context->stack_begin(), m_context->stack_pos() - output);
						for (; it != m_context->stack_pos(); it++)
						{
							printf("\n   %s", inspect(*it).c_str());
						}
					}
					printf(" <--\n");
					printf("----------------\n");
				}
				DO_DEBUG {
					if ((long)output != -1)
						assert(m_context->stack_count() == expected);
				}

				it = m_context->next();
			}
			DO_TRACE printf("Exiting running state with context %p\n", m_context);
			m_running = false;
			m_context = NULL;
		}
	}
}