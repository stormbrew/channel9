#pragma once

#include "c9/channel9.hpp"
#include "c9/environment.hpp"
#include "c9/callable_context.hpp"
#include "c9/istream.hpp"
#include "c9/gc.hpp"
#include "c9/variable_frame.hpp"
#include "c9/message.hpp"

#include <assert.h>
#include <stdio.h>
#include <vector>

namespace Channel9
{
	struct RunnableContext
	{
		const static ValueType gc_type_id = RUNNABLE_CONTEXT;

		IStream *m_instructions;
		const Instruction *m_pos;
		VariableFrame *m_lexicalvars;
		size_t m_stack_pos;
		RunningContext *m_caller;

		Value m_data[0];

		void jump(size_t pos)
		{
			m_pos = &*m_instructions->begin() + pos;
		}

		std::string inspect() const
		{
			return m_instructions->source_pos(m_pos).inspect();
		}

		void new_scope(VariableFrame *scope) { m_lexicalvars = scope; }
	};

	struct RunningContext
	{
		const static ValueType gc_type_id = RUNNING_CONTEXT;

		IStream *m_instructions;
		const Instruction *m_pos;
		VariableFrame *m_lexicalvars;
		size_t m_stack_pos;
		RunningContext *m_caller;

		Value m_data[0];

		std::string inspect() const
		{
			return m_instructions->source_pos(m_pos).inspect();
		}

		const IStream &instructions() { return *m_instructions; }
		const Instruction *next() { return m_pos++; }
		const Instruction *peek() const { return m_pos; }
		const Instruction *end() const { return &*m_instructions->end(); }
		void invalidate() { m_pos = NULL; }

		void jump(const std::string &label);
		void jump(size_t pos)
		{
			m_pos = &*m_instructions->begin() + pos;
		}

		const Value *stack_begin() const { return m_data + m_instructions->stack_offset(); }
		const Value *stack_pos() const { return m_data + m_stack_pos; }
		size_t stack_count() const { return m_stack_pos - m_instructions->stack_offset(); }
		void push(const Value &val)
		{
			gc_write_value(this, m_data[m_stack_pos++], val);
		}
		void pop()
		{
			--m_stack_pos;
		}
		const Value &top() const { return m_data[m_stack_pos-1]; }

		const Value &get_framevar(size_t id) const { return m_data[id]; }
		const Value &get_localvar(size_t id) const { return m_data[m_instructions->local_offset() + id]; }
		const Value &get_lexicalvar(size_t id) const { return m_lexicalvars->lookup(id); }
		const Value &get_lexicalvar(size_t id, size_t depth) const { return m_lexicalvars->lookup(id, depth); }

		void set_framevar(size_t id, const Value &val)
		{
			gc_write_value(this, m_data[id], val);
		}
		void set_localvar(size_t id, const Value &val)
		{
			gc_write_value(this, m_data[m_instructions->local_offset() + id], val);
		}
		void set_lexicalvar(size_t id, const Value &val) { m_lexicalvars->set(id, val); }
		void set_lexicalvar(size_t id, size_t depth, const Value &val) { m_lexicalvars->set(id, depth, val); }

		void new_scope(VariableFrame *scope)
		{
			gc_write_ptr(this, m_lexicalvars, scope);
		}

		void debug_print_backtrace(size_t max = -1) const;
	};

	inline RunnableContext *new_context(IStream *instructions, VariableFrame *lexicalvars = NULL, size_t pos = 0)
	{
		size_t frame_count = instructions->frame_count();

		RunnableContext *ctx = gc_alloc<RunnableContext>(sizeof(Value)*instructions->frame_count());
		ctx->m_instructions = instructions;
		ctx->m_pos = &*instructions->begin();
		ctx->m_lexicalvars = lexicalvars;
		ctx->m_stack_pos = frame_count;
		memset(ctx->m_data, 0, sizeof(Value)*instructions->frame_count());
		return ctx;
	}
	inline RunnableContext *new_context(const RunnableContext &copy)
	{
		nursery_pool.validate(&copy);
		size_t frame_count = copy.m_instructions->frame_count();
		RunnableContext *ctx = gc_alloc<RunnableContext>(sizeof(Value)*(frame_count));
		memcpy(ctx, &copy, sizeof(RunnableContext) + sizeof(Value)*frame_count);
		return ctx;
	}
	inline RunnableContext *new_context(const Value &copy)
	{
		size_t frame_count = ptr<RunnableContext>(copy)->m_instructions->frame_count();
		RunnableContext *ctx = gc_alloc<RunnableContext>(sizeof(Value)*(frame_count));
		memcpy(ctx, ptr<RunnableContext>(copy), sizeof(RunnableContext) + sizeof(Value)*frame_count);
		return ctx;
	}
	inline RunningContext *activate_context(RunningContext *caller, const Value &copy)
	{
		IStream *istream = ptr<RunnableContext>(copy)->m_instructions;
		size_t frame_count = istream->frame_count();
		size_t frame_extra = sizeof(Value)*(istream->frame_size());
		RunningContext *ctx = gc_alloc<RunningContext>(frame_extra);

		memcpy(ctx, ptr<RunnableContext>(copy), sizeof(RunnableContext) + sizeof(Value)*frame_count);
		std::fill(ctx->m_data + istream->local_offset(), ctx->m_data + istream->stack_offset(), Nil);
		ctx->m_stack_pos = istream->stack_offset();
		ctx->m_caller = caller;
		return ctx;
	}

	typedef void (send_func)(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	extern send_func *send_types[0xff];

#	define INIT_SEND_FUNC(type, func) \
		static send_func *_send_func_##type = send_types[type] = func;

	inline void channel_send(Environment *env, const Value &channel, const Value &val, const Value &ret)
	{
		return send_types[type(channel)](env, ret, channel, val);
	}
	void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Value &msg);
}

#include "c9/primitive.hpp"

