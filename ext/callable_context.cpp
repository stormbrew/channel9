#include "callable_context.hpp"
#include "context.hpp"
#include "environment.hpp"

#include "time.hpp"

namespace Channel9
{
	std::vector<CallableContext*> CallableContext::contexts;
	bool CallableContext::sweep_flag = false;

	static void callable_context_send(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	INIT_SEND_FUNC(CALLABLE_CONTEXT, &callable_context_send);

	void callable_context_send(Environment *env, const Value &ret, const Value &oself, const Value &msg)
	{
		ptr<CallableContext>(oself)->send(env, msg, ret);
	}

	void CallableContext::scan()
	{
		m_sweep = !sweep_flag;
	}

	void CallableContext::sweep()
	{
		size_t total = 0, swept = 0;
		Time start_time;
		TRACE_DO(TRACE_GC, TRACE_INFO) {
			total = contexts.size();
		}
		context_set fresh_contexts;
		fresh_contexts.reserve(contexts.size());

		sweep_flag = !sweep_flag;
		context_set::iterator it = contexts.begin();
		while (it != contexts.end())
		{
			CallableContext *obj = *it++;
			if (obj->m_sweep != sweep_flag)
			{
				TRACE_DO(TRACE_GC, TRACE_INFO) ++swept;
				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Deleting CallableContext %p (%s)\n", obj, obj->inspect().c_str());
				delete obj;
			} else {
				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Keeping CallableContext %p (%s)\n", obj, obj->inspect().c_str());
				fresh_contexts.push_back(obj);
			}
		}
		swap(contexts, fresh_contexts);
		double timediff = Time() - start_time;
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Deleted %i out of %i CallableContexts, leaving %i. Took %.3fs, %fs/obj\n",
			int(swept), int(total), int(total - swept), timediff, timediff/total);
	}
}
