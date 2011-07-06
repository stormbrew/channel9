#pragma once

#include "channel9.hpp"

#include "istream.hpp"

namespace Channel9
{
	class VariableFrame
	{
	private:
		Value::vector m_locals;
		VariableFrame *m_parent_frame;

	protected:
		VariableFrame *parent() { return m_parent_frame; }
		const VariableFrame *parent() const { return m_parent_frame; }
		VariableFrame *parent_depth(size_t depth);
		const VariableFrame *parent_depth(size_t depth) const;

	public:
		VariableFrame(size_t local_count, VariableFrame *parent = NULL)
		 : m_locals(local_count, Value::Undef), m_parent_frame(parent)
		{}

		const Value &lookup(size_t id) const { return m_locals[id]; }
		const Value &lookup(size_t id, size_t depth) const;
		void set(size_t id, const Value &val) { m_locals[id] = val; }
		void set(size_t id, size_t depth, const Value &val);
	};

	class CallableContext
	{
	public:
		virtual void send(Environment *env, const Value &val, const Value &ret) = 0;
	};

	class BytecodeContext : public CallableContext
	{
	private:
		Environment *m_environment;
		IStream *m_instructions;
		IStream::iterator m_pos;
		Value::vector m_framevars;
		VariableFrame *m_localvars;

	public:
		BytecodeContext(Environment *env, IStream *instructions, 
			const Value::vector *framevars = NULL, VariableFrame *localvars = NULL);
		BytecodeContext(Environment *env, IStream *instructions, IStream::iterator pos,
			const Value::vector *framevars = NULL, VariableFrame *localvars = NULL);

		Environment *env() const { return m_environment; }

		virtual void send(Environment *cenv, const Value &val, const Value &ret);
	};

	class RunnableContext
	{
	private:
		Environment *m_environment;
		IStream *m_instructions;
		IStream::iterator m_pos;
		Value::vector m_framevars;
		VariableFrame *m_localvars;

		Value::vector m_stack;

	public:
		RunnableContext(Environment *env, IStream *instructions, IStream::iterator pos,
			const Value::vector *framevars = NULL, VariableFrame *localvars = NULL);

		Environment *env() const { return m_environment; }
		
		void send(Environment *cenv, const Value &val, const Value &ret);
	
		const IStream &instructions() { return *m_instructions; }
		IStream::const_iterator next() { return m_pos++; }
		IStream::const_iterator peek() const { return m_pos; }
		IStream::const_iterator end() const { return m_instructions->end(); }

		void jump(const std::string &label);
		CallableContext *new_context(const std::string &label);

		size_t stack_count() const { return m_stack.size(); }
		void push(const Value &val) { m_stack.push_back(val); }
		void pop() { m_stack.pop_back(); }
		const Value &top() const { return m_stack.back(); }

		const Value &get_framevar(size_t id) const { return m_framevars[id]; }
		const Value &get_localvar(size_t id) const { return m_localvars->lookup(id); }
		const Value &get_localvar(size_t id, size_t depth) const { return m_localvars->lookup(id, depth); }

		void set_framevar(size_t id, const Value &val) { m_framevars[id] = val; }
		void set_localvar(size_t id, const Value &val) { m_localvars->set(id, val); }
		void set_localvar(size_t id, size_t depth, const Value &val) { m_localvars->set(id, depth, val); }

		void new_scope(bool linked = false);
	};
}