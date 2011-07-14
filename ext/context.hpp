#pragma once

#include "channel9.hpp"
#include "environment.hpp"
#include "istream.hpp"

#include <assert.h>

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

		const Value &lookup(size_t id) const { return *(m_locals.begin() + id); }
		const Value &lookup(size_t id, size_t depth) const;
		void set(size_t id, const Value &val) { *(m_locals.begin() + id) = val; }
		void set(size_t id, size_t depth, const Value &val);
	};

	class CallableContext
	{
	public:
		virtual void send(Environment *env, const Value &val, const Value &ret) = 0;
		virtual ~CallableContext() = 0;
	};

	class RunnableContext
	{
	private:
		Environment *m_environment;
		IStream *m_instructions;
		IStream::iterator m_pos;
		Value::vector m_framevars;
		VariableFrame *m_localvars;

		Value::vector *m_stack;
		Value::vector::iterator m_sp;

	public:
		RunnableContext(Environment *env, IStream *instructions,
			const Value::vector *framevars = NULL, VariableFrame *localvars = NULL);
		RunnableContext(Environment *env, IStream *instructions, IStream::iterator pos,
			const Value::vector *framevars = NULL, VariableFrame *localvars = NULL);
		
		Environment *env() const { return m_environment; }
		
		void send(Environment *cenv, const Value &val, const Value &ret);
	
		const IStream &instructions() { return *m_instructions; }
		IStream::const_iterator next() { return m_pos++; }
		IStream::const_iterator peek() const { return m_pos; }
		IStream::const_iterator end() const { return m_instructions->end(); }

		void jump(const std::string &label);
		void jump(size_t pos);

		void associate_stack();
		void dissociate_stack() { m_stack = NULL; }
		Value::vector::const_iterator stack_begin() const { return m_stack->begin(); }
		Value::vector::const_iterator stack_pos() const { return m_sp; }
		size_t stack_count() const { return m_sp - m_stack->begin(); }
		void push(const Value &val) 
		{ 
			DO_DEBUG assert(m_sp < m_stack->end());
			*m_sp++ = val; 
		}
		void pop() 
		{ 
			--m_sp; 
			DO_DEBUG assert(m_sp >= m_stack->begin());
		}
		const Value &top() const { return *(m_sp-1); }

		const Value &get_framevar(size_t id) const { return *(m_framevars.begin() + id); }
		const Value &get_localvar(size_t id) const { return m_localvars->lookup(id); }
		const Value &get_localvar(size_t id, size_t depth) const { return m_localvars->lookup(id, depth); }

		void set_framevar(size_t id, const Value &val) { m_framevars[id] = val; }
		void set_localvar(size_t id, const Value &val) { m_localvars->set(id, val); }
		void set_localvar(size_t id, size_t depth, const Value &val) { m_localvars->set(id, depth, val); }

		void new_scope(bool linked = false);
	};

	inline void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Message &msg);
	inline void number_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg);
	inline void string_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg);
	inline void tuple_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg);
	inline void channel_send(Environment *env, const Value &channel, const Value &val, const Value &ret)
	{
		switch (channel.m_type)
		{
		case RUNNABLE_CONTEXT:
			channel.ret_ctx->send(env, val, ret);
			break;
		case CALLABLE_CONTEXT:
			channel.call_ctx->send(env, val, ret);
			break;
		case NIL: {
			const Value &def = env->special_channel("Channel9::Primitive::NilC");
			return forward_primitive_call(env, def, ret, channel, *val.msg);
			}
		case UNDEF: {
			const Value &def = env->special_channel("Channel9::Primitive::UndefC");
			return forward_primitive_call(env, def, ret, channel, *val.msg);
			}
		case BFALSE: {
			const Value &def = env->special_channel("Channel9::Primitive::TrueC");
			return forward_primitive_call(env, def, ret, channel, *val.msg);
			}
		case BTRUE: {
			const Value &def = env->special_channel("Channel9::Primitive::FalseC");
			return forward_primitive_call(env, def, ret, channel, *val.msg);
			}
		case MESSAGE: {
			const Value &def = env->special_channel("Channel9::Primitive::Message");
			return forward_primitive_call(env, def, ret, channel, *val.msg);
			}
		case MACHINE_NUM:
			number_channel_simple(env, ret, channel, *val.msg);
			break;
		case STRING:
			string_channel_simple(env, ret, channel, *val.msg);
			break;
		case TUPLE:
			tuple_channel_simple(env, ret, channel, *val.msg);
			break;
		default:
			printf("Built-in Channel for %d not yet implemented.\n", channel.m_type);
			exit(1);
		}
	}
}

#include "primitive.hpp"