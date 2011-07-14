#include "context.hpp"
#include "environment.hpp"

namespace Channel9
{
	RunnableContext::RunnableContext(Environment *env, IStream *instructions,
		const Value::vector *framevars, VariableFrame *localvars)
	 : m_environment(env), m_instructions(instructions), m_pos(instructions->begin()),
	   m_framevars(), m_localvars(localvars), m_stack(NULL)
	{
		if (!m_localvars)
		{
			m_localvars = new VariableFrame(instructions->local_count());
		}
		if (framevars)
		{
			m_framevars = *framevars;
		} else {
			m_framevars = Value::vector(instructions->frame_count(), Value::Undef);
		}
	}
	RunnableContext::RunnableContext(Environment *env, IStream *instructions, IStream::iterator pos,
		const Value::vector *framevars, VariableFrame *localvars)
	 : m_environment(env), m_instructions(instructions), m_pos(pos),
	   m_framevars(), m_localvars(localvars), m_stack(NULL)
	{
		if (!m_localvars)
		{
			m_localvars = new VariableFrame(instructions->local_count());
		}
		if (framevars)
		{
			m_framevars = *framevars;
		} else {
			m_framevars = Value::vector(instructions->frame_count(), Value::Undef);
		}
	}

	void RunnableContext::associate_stack()
	{
		m_stack = new Value::vector(m_instructions->stack_size(), Value::Undef);
		m_sp = m_stack->begin();
	}

	void RunnableContext::jump(const std::string &label)
	{
		m_pos = m_instructions->begin() + m_instructions->label_pos(label);
	}
	void RunnableContext::jump(size_t pos)
	{
		m_pos = m_instructions->begin() + pos;
	}
	
	void RunnableContext::send(Environment *cenv, const Value &val, const Value &ret)
	{
		if (m_stack)
		{
			push(val);
			push(ret);
			m_environment->run(this);
		} else {
			RunnableContext *nctx = new RunnableContext(m_environment, m_instructions,
				m_pos, &m_framevars, m_localvars);
			nctx->associate_stack();
			nctx->send(cenv, val, ret);
		}
	}

	void RunnableContext::new_scope(bool linked)
	{
		m_localvars = new VariableFrame(m_instructions->local_count(), m_localvars);
	}

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
			return Value::Nil;
	}
	void VariableFrame::set(size_t id, size_t depth, const Value &val)
	{
		VariableFrame *frame = parent_depth(depth);
		if (frame)
			frame->set(id, val);
	}

}