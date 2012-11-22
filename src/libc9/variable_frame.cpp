#include "c9/channel9.hpp"
#include "c9/variable_frame.hpp"

namespace Channel9
{
	const VariableFrame *VariableFrame::parent_depth(size_t depth) const
	{
		const VariableFrame *t = this;
		while (depth > 0 && t)
		{
			t = t->parent();
			depth--;
		}
		return t;
	}
	VariableFrame *VariableFrame::parent_depth(size_t depth)
	{
		VariableFrame *t = this;
		while (depth > 0 && t)
		{
			t = t->parent();
			depth--;
		}
		return t;
	}

	const Value &VariableFrame::lookup(size_t id, size_t depth) const
	{
		const VariableFrame *frame = parent_depth(depth);
		if (frame)
			return frame->lookup(id);
		else
			return Nil;
	}
	void VariableFrame::set(size_t id, size_t depth, const Value &val)
	{
		VariableFrame *frame = parent_depth(depth);
		if (frame)
			frame->set(id, val);
	}

	static void scan_variable_frame(VariableFrame *from)
	{
		size_t lexical_count = from->m_instructions->lexical_count();
		gc_scan(from->m_instructions);
		if (from->m_parent_frame)
			value_pool.mark((void**)&from->m_parent_frame);

		for (size_t i = 0; i < lexical_count; i++)
		{
			gc_scan(from->m_lexicals[i]);
		}
	}
	INIT_SCAN_FUNC(VARIABLE_FRAME, &scan_variable_frame);
}
