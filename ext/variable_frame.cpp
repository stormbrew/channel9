#include "channel9.hpp"
#include "variable_frame.hpp"

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
}
