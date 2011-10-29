#include "context.hpp"
#include "environment.hpp"

#include <time.h>

namespace Channel9
{
	send_func *send_types[0xff];

	static void running_context_send(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	static void runnable_context_send(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	INIT_SEND_FUNC(RUNNING_CONTEXT, &running_context_send);
	INIT_SEND_FUNC(RUNNABLE_CONTEXT, &runnable_context_send);

	void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Value &msg)
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

	void running_context_send(Environment *env, const Value &ret, const Value &oself, const Value &msg)
	{
		RunningContext *ctx = ptr<RunningContext>(oself);
		assert(ctx->m_pos);

		ctx->push(msg);
		ctx->push(ret);
		env->run(ctx);
	}
	void runnable_context_send(Environment *env, const Value &ret, const Value &oself, const Value &msg)
	{
		RunningContext *caller;
		if (is(ret, RUNNING_CONTEXT))
			caller = ptr<RunningContext>(ret);
		else
			caller = env->context();
			
		RunningContext *ctx = activate_context(caller, oself);

		ctx->push(msg);
		ctx->push(ret);
		env->run(ctx);
	}

	void RunningContext::jump(const std::string &label)
	{
		m_pos = &*m_instructions->begin() + m_instructions->label_pos(label);
	}

	void RunningContext::debug_print_backtrace(size_t max) const
	{
		const RunningContext *ctx = this;
		while (max > 0 && ctx)
		{
			SourcePos pos = ctx->m_instructions->source_pos(ctx->m_pos);
			printf("%s:%llu:%llu (%s)\n", pos.file.c_str(), (uint64_t)pos.line_num, (uint64_t)pos.column, pos.annotation.c_str());
			ctx = ctx->m_caller;
			--max;
		}
	}
}