#include "context.hpp"
#include "environment.hpp"

#include <time.h>

namespace Channel9
{
	std::vector<CallableContext*> CallableContext::contexts;
	bool CallableContext::sweep_flag = false;

	void CallableContext::scan()
	{
		m_sweep = !sweep_flag;
	}

	void CallableContext::sweep()
	{
		size_t total = 0, swept = 0;
		time_t start = 0;
		TRACE_DO(TRACE_GC, TRACE_INFO) {
			total = contexts.size();
			start = time(NULL);
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
				TRACE_PRINTF(TRACE_GC, TRACE_DEBUG, "Deleting CallableContext %p\n", obj);
				delete obj;
			} else {
				fresh_contexts.push_back(obj);
			}
		}
		swap(contexts, fresh_contexts);
		TRACE_PRINTF(TRACE_GC, TRACE_INFO, "Deleted %i out of %i CallableContexts, leaving %i. Took %is, %fs/obj\n", 
			int(swept), int(total), int(total - swept), int(time(NULL)-start), double(time(NULL)-start)/total);
	}

	void RunningContext::jump(const std::string &label)
	{
		m_pos = &*m_instructions->begin() + m_instructions->label_pos(label);
	}
}