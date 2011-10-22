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

	void running_context_send(Environment *env, const Value &ret, const Value &oself, const Value &msg)
	{
		RunningContext *ctx = ptr<RunningContext>(oself);

		ctx->push(msg);
		ctx->push(ret);
		env->run(ctx);
	}
	void runnable_context_send(Environment *env, const Value &ret, const Value &oself, const Value &msg)
	{
		RunningContext *ctx = activate_context(env->context(), oself);

		ctx->push(msg);
		ctx->push(ret);
		env->run(ctx);
	}

	void RunningContext::jump(const std::string &label)
	{
		m_pos = &*m_instructions->begin() + m_instructions->label_pos(label);
	}
}