#include "environment.hpp"
#include "value.hpp"
#include "istream.hpp"
#include "context.hpp"
#include "message.hpp"
#include "primitive.hpp"

#include <assert.h>

namespace Channel9
{
	class NoReturnContext : public CallableContext
	{
	public:
		virtual void send(Environment *env, const Value &val, const Value &ret)
		{
			// TODO: Make this do something more sensible
			printf("Trap: Tried to return to unreturnable context.");
			exit(1);
		}
		virtual ~NoReturnContext() {}
	};

	static NoReturnContext no_return_ctx;

	Environment::Environment()
	 : GCRoot(value_pool), m_context(NULL), m_running(false),
	   m_vstack(), m_vspos(0)
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
		TRACE_PRINTF(TRACE_VM, TRACE_INFO, "set_special_channel(%s, %s)\n", name.c_str(), inspect(val).c_str());
		m_specials[name] = val;
	}

	void Environment::scan()
	{
		if (m_context)
			value_pool.mark(&m_context);

		gc_scan(&m_ipos);

		for (size_t i = 0; i < m_vspos; i++)
		{
			gc_scan(m_vstack[i]);
		}

		for (special_map::iterator it = m_specials.begin(); it != m_specials.end(); it++)
		{
			gc_scan(it->second);
		}

		no_return_ctx.scan();
	}

	void Environment::run(RunningContext *context)
	{
		const Instruction &ins = m_ipos;
		m_context = context;
		if (!context)
		{
			m_running = false;
		} else if (!m_running)
		{
			clear_vstore();
			TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Entering running state with %p\n", context);
			m_running = true;
			const Instruction *ipos = m_context->next();
			SourcePos *last_pos = NULL; // used for debug output below.
			while (m_context && ipos != m_context->end())
			{
				size_t output = -1;
				size_t expected = -1;
#				define CHECK_STACK(in, out) TRACE_OUT(TRACE_VM, TRACE_DEBUG) {\
					assert(m_context->stack_count() >= (size_t)(in)); \
					if ((out) != -1) { assert((long)(expected = m_context->stack_count() - (size_t)(in) + (size_t)(out)) >= 0); } \
					tprintf("Stack info: before(%d), in(%d), out(%d), expected(%d)\n", (int)m_context->stack_count(), (int)(in), (int)(out), (int)expected); \
					output = (out); \
				}

				m_ipos = *ipos;

				TRACE_DO(TRACE_VM, TRACE_INFO) {
					SourcePos &pos = m_context->m_instructions->source_pos(ipos);
					if (!last_pos || *last_pos != pos)
						tprintf("Source Position: %s:%d:%d (%s)\n", pos.file.c_str(), (int)pos.line_num, (int)pos.column, pos.annotation.c_str());
					last_pos = &pos;
				}
				TRACE_OUT(TRACE_VM, TRACE_DEBUG) {
					tprintf("Instruction: %s@%d\n",
						inspect(ins).c_str(),
						(int)(ipos - &*m_context->instructions().begin())
						);
					tprintf("Stack: %d deep", (int)m_context->stack_count());

					const Value *it;
					for (it = m_context->stack_begin(); it != m_context->stack_pos(); it++)
					{
						tprintf("\n   %s", inspect(*it).c_str());
					}
					tprintf(" <--\n");
				}

				switch(ins.instruction)
				{
				case NOP:
					CHECK_STACK(0, 0);
					break;
				case DEBUGGER:
					CHECK_STACK(0, 0);
					DO_DEBUG __asm__("int3");
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

					value_pool.safe_point();

					m_context->jump(ins.cache.machine_num);
					break;
				case JMP_IF:
					CHECK_STACK(1, 0);

					value_pool.safe_point();

					if (is_truthy(m_context->top()))
						m_context->jump(ins.cache.machine_num);
					m_context->pop();
					break;
				case JMP_IF_NOT:
					CHECK_STACK(1, 0);

					value_pool.safe_point();

					if (!is_truthy(m_context->top()))
						m_context->jump(ins.cache.machine_num);
					m_context->pop();
					break;

				case LEXICAL_CLEAN_SCOPE: {
					CHECK_STACK(0, 0);
					VariableFrame *frame = new_variable_frame(m_context->m_instructions);
					m_context->new_scope(frame);
					}
					break;
				case LEXICAL_LINKED_SCOPE: {
					CHECK_STACK(0, 0);
					VariableFrame *frame = new_variable_frame(m_context->m_instructions);
					frame->link_frame(m_context->m_lexicalvars);
					m_context->new_scope(frame);
					}
					break;
				case FRAME_GET: {
					CHECK_STACK(0, 1);
					size_t frameid = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Getting frame var %s (%d)\n", ptr<String>(ins.arg1)->c_str(), (int)frameid);
					m_context->push(m_context->get_framevar(frameid));
					}
					break;
				case FRAME_SET: {
					CHECK_STACK(1, 0);
					size_t frameid = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Setting frame var %s (%d) to: %s\n", ptr<String>(ins.arg1)->c_str(), (int)frameid, inspect(m_context->top()).c_str());
					m_context->set_framevar(frameid, m_context->top());
					m_context->pop();
					}
					break;
				case LOCAL_GET: {
					CHECK_STACK(0, 1);
					size_t localid = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Getting local var %s (%d)\n", ptr<String>(ins.arg1)->c_str(), (int)localid);
					m_context->push(m_context->get_localvar(localid));
					}
					break;
				case LOCAL_SET: {
					CHECK_STACK(1, 0);
					size_t localid = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Setting local var %s (%d) to: %s\n", ptr<String>(ins.arg1)->c_str(), (int)localid, inspect(m_context->top()).c_str());
					m_context->set_localvar(localid, m_context->top());
					m_context->pop();
					}
					break;
				case LEXICAL_GET: {
					CHECK_STACK(0, 1);
					size_t depth = ins.arg1.machine_num;
					size_t localid = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "lexical_get %u@%u: %i\n", (unsigned)localid, (unsigned)depth, type(m_context->get_lexicalvar(localid, depth)));
					if (depth == 0)
						m_context->push(m_context->get_lexicalvar(localid));
					else
						m_context->push(m_context->get_lexicalvar(localid, depth));
					}
					break;
				case LEXICAL_SET: {
					CHECK_STACK(1, 0);
					size_t depth = ins.arg1.machine_num;
					size_t localid = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "lexical_set %u@%u: %i\n", (unsigned)localid, (unsigned)depth, type(m_context->top()));
					if (depth == 0)
						m_context->set_lexicalvar(localid, m_context->top());
					else
						m_context->set_lexicalvar(localid, depth, m_context->top());
					m_context->pop();
					}
					break;

				case CHANNEL_NEW: {
					CHECK_STACK(0, 1);
					const Value &octx = vstore(value(m_context));
					RunnableContext *nctx = new_context(octx);
					nctx->jump(ins.cache.machine_num);
					m_context->push(value(nctx));
					clear_vstore();
					}
					break;
				case CHANNEL_SPECIAL:
					CHECK_STACK(0, 1);
					m_context->push(special_channel(*ptr<String>(ins.arg1)));
					break;
				case CHANNEL_SEND: {
					CHECK_STACK(3, -1);

					value_pool.safe_point();

					const Value &val = vstore(m_context->top()); m_context->pop();
					const Value &ret = vstore(m_context->top()); m_context->pop();
					const Value &channel = vstore(m_context->top()); m_context->pop();

					channel_send(this, channel, val, ret);
					clear_vstore();
					}
					break;
				case CHANNEL_CALL:{
					CHECK_STACK(2, -1);

					value_pool.safe_point();

					const Value &val = vstore(m_context->top()); m_context->pop();
					const Value &channel = vstore(m_context->top()); m_context->pop();
					const Value &ret = vstore(value(m_context));

					channel_send(this, channel, val, ret);
					clear_vstore();
					}
					break;
				case CHANNEL_RET:{
					CHECK_STACK(2, -1);
					const Value &val = vstore(m_context->top()); m_context->pop();
					const Value &channel = vstore(m_context->top()); m_context->pop();

					channel_send(this, channel, val, value(&no_return_ctx));
					clear_vstore();
					}
					break;

				case MESSAGE_NEW: {
					long long id = ins.cache.machine_num;
					long long sysarg_count = ins.arg2.machine_num, sysarg_counter = sysarg_count - 1;
					long long arg_count = ins.arg3.machine_num, arg_counter = arg_count - 1;
					CHECK_STACK(sysarg_count + arg_count, 1);

					Message *msg = new_message(id, sysarg_count, arg_count);

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

					Message *nmsg = new_message(msg.m_id, msg.sysarg_count(), msg.arg_count() + tuple.size());
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

					Message *nmsg = new_message(msg.m_id, msg.sysarg_count(), msg.arg_count() + count);
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
				case MESSAGE_IS:
					CHECK_STACK(1, 2);
					m_context->push(bvalue(ptr<Message>(m_context->top())->m_id == (uint64_t)ins.cache.machine_num));
					break;
				case MESSAGE_IS_PROTO:
					CHECK_STACK(1, 2);
					m_context->push(bvalue(ptr<Message>(m_context->top())->protocol_id() == (uint64_t)ins.cache.machine_num));
					break;
				case MESSAGE_ID:
					{
					CHECK_STACK(1, 2);
					Message *msg = ptr<Message>(m_context->top());
					m_context->push(value((long long)msg->m_id));
					}
					break;
				case MESSAGE_SPLIT_ID:
					{
					CHECK_STACK(1, 3);
					Message *msg = ptr<Message>(m_context->top());
					m_context->push(value((long long)msg->message_id()));
					m_context->push(value((long long)msg->protocol_id()));
					}
					break;
				case MESSAGE_CHECK:
					CHECK_STACK(1, 1);
					if (!is(m_context->top(), MESSAGE))
					{
						m_context->pop();
						m_context->push(False);
					}
					m_context->push(value(ptr<Message>(m_context->top())->name()));
					break;
				case MESSAGE_FORWARD: {
					CHECK_STACK(1, 1);

					long long id = ins.cache.machine_num;
					const Message &msg = *ptr<Message>(m_context->top());
					Message *nmsg = new_message(id, msg.sysarg_count(), msg.arg_count() + 1);
					std::copy(msg.sysargs(), msg.sysargs_end(), nmsg->sysargs());
					nmsg->args()[0] = value(new_string(msg.name()));
					std::copy(msg.args(), msg.args_end(), nmsg->args() + 1);
					m_context->pop();
					m_context->push(value(nmsg));
					break;
				}
				case MESSAGE_SYS_PREFIX: {
					long long count = ins.arg1.machine_num, counter = 0;
					CHECK_STACK(1 + count, 1);
					
					const Value *sdata = m_context->stack_pos() - 1 - count;
					const Message &msg = *ptr<Message>(*sdata++);
					Message *nmsg = new_message(msg.m_id, msg.sysarg_count() + count, msg.arg_count());
					Message::iterator to = nmsg->sysargs();
					while (counter++ < count)
					{
						*to++ = *sdata++;
					}
					std::copy(msg.sysargs(), msg.sysargs_end(), to);
					std::copy(msg.args(), msg.args_end(), nmsg->args());
					while (--counter != 0)
					{
						m_context->pop();
					}
					m_context->pop(); // message we read off stack before.
					m_context->push(value(nmsg));
					break;
				}
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
					const Value &coerce = ins.arg1;
					const Value &val = vstore(m_context->top());
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
					clear_vstore();
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

				TRACE_OUT(TRACE_VM, TRACE_DEBUG) {
					tprintf("Output: %d", (int)output);
					if ((long)output > 0)
					{
						const Value *it = std::max(
							m_context->stack_begin(), m_context->stack_pos() - output);
						for (; it != m_context->stack_pos(); it++)
						{
							tprintf("\n   %s", inspect(*it).c_str());
						}
					}
					tprintf(" <--\n");
					tprintf("----------------\n");
				}
				DO_DEBUG {
					if ((long)output != -1)
						assert(m_context->stack_count() == expected);
				}

				ipos = m_context->next();
			}
			TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Exiting running state with context %p\n", m_context);
			m_running = false;
			m_context = NULL;
		}
	}
}

