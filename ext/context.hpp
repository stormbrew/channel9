#pragma once

#include "channel9.hpp"
#include "environment.hpp"
#include "istream.hpp"
#include "memory_pool.hpp"
#include "variable_frame.hpp"

#include <assert.h>
#include <stdio.h>

namespace Channel9
{
	class CallableContext
	{
	public:
		virtual void send(Environment *env, const Value &val, const Value &ret) = 0;
		virtual void scan() = 0;
		virtual ~CallableContext() {};
	};

	inline void gc_scan(CallableContext *ctx)
	{
		ctx->scan();
	}

	// Specialize GCRef's scanner to only do a gc_scan of it.
	template <>
	inline void GCRef<CallableContext*>::scan()
	{
		gc_scan(m_val);
	}

	struct RunnableContext
	{
		IStream *m_instructions;
		const Instruction *m_pos;
		VariableFrame *m_localvars;
		size_t m_stack_pos;
		RunningContext *m_caller;

		Value m_data[0];

		void jump(size_t pos)
		{
			m_pos = &*m_instructions->begin() + pos;
		}

		void new_scope(VariableFrame *scope) { m_localvars = scope; }
	};

	struct RunningContext
	{
		IStream *m_instructions;
		const Instruction *m_pos;
		VariableFrame *m_localvars;
		size_t m_stack_pos;
		RunningContext *m_caller;

		Value m_data[0];

		const IStream &instructions() { return *m_instructions; }
		const Instruction *next() { return m_pos++; }
		const Instruction *peek() const { return m_pos; }
		const Instruction *end() const { return &*m_instructions->end(); }

		void jump(const std::string &label);
		void jump(size_t pos)
		{
			m_pos = &*m_instructions->begin() + pos;
		}

		const Value *stack_begin() const { return m_data + m_instructions->frame_count(); }
		const Value *stack_pos() const { return m_data + m_stack_pos; }
		size_t stack_count() const { return m_stack_pos - m_instructions->frame_count(); }
		void push(const Value &val)
		{
			m_data[m_stack_pos++] = val;
		}
		void pop()
		{
			--m_stack_pos;
		}
		const Value &top() const { return m_data[m_stack_pos-1]; }

		const Value &get_framevar(size_t id) const { return m_data[id]; }
		const Value &get_localvar(size_t id) const { return m_localvars->lookup(id); }
		const Value &get_localvar(size_t id, size_t depth) const { return m_localvars->lookup(id, depth); }

		void set_framevar(size_t id, const Value &val) { m_data[id] = val; }
		void set_localvar(size_t id, const Value &val) { m_localvars->set(id, val); }
		void set_localvar(size_t id, size_t depth, const Value &val) { m_localvars->set(id, depth, val); }

		void new_scope(VariableFrame *scope) { m_localvars = scope; }
	};

	inline RunnableContext *new_context(IStream *instructions, VariableFrame *localvars = NULL, size_t pos = 0)
	{
		size_t frame_count = instructions->frame_count();

		RunnableContext *ctx = value_pool.alloc<RunnableContext>(sizeof(Value)*instructions->frame_count(), RUNNABLE_CONTEXT);
		ctx->m_instructions = instructions;
		ctx->m_pos = &*instructions->begin();
		ctx->m_localvars = localvars;
		ctx->m_stack_pos = frame_count;
		memset(ctx->m_data, 0, sizeof(Value)*instructions->frame_count());
		return ctx;
	}
	inline RunnableContext *new_context(const RunnableContext &copy)
	{
		value_pool.validate(&copy);
		size_t frame_count = copy.m_instructions->frame_count();
		RunnableContext *ctx = value_pool.alloc<RunnableContext>(sizeof(Value)*(frame_count), RUNNABLE_CONTEXT);
		memcpy(ctx, &copy, sizeof(RunnableContext) + sizeof(Value)*frame_count);
		return ctx;
	}
	inline RunnableContext *new_context(const Value &copy)
	{
		size_t frame_count = ptr<RunnableContext>(copy)->m_instructions->frame_count();
		RunnableContext *ctx = value_pool.alloc<RunnableContext>(sizeof(Value)*(frame_count), RUNNABLE_CONTEXT);
		memcpy(ctx, ptr<RunnableContext>(copy), sizeof(RunnableContext) + sizeof(Value)*frame_count);
		return ctx;
	}
	inline RunningContext *activate_context(RunningContext *caller, const Value &copy)
	{
		IStream *istream = ptr<RunnableContext>(copy)->m_instructions;
		size_t frame_count = istream->frame_count();
		size_t frame_extra = sizeof(Value)*(frame_count + istream->stack_size());
		RunningContext *ctx = value_pool.alloc<RunningContext>(frame_extra, RUNNING_CONTEXT);

		memcpy(ctx, ptr<RunnableContext>(copy), sizeof(RunnableContext) + sizeof(Value)*frame_count);
		ctx->m_stack_pos = frame_count;
		ctx->m_caller = caller;
		return ctx;
	}

	inline void gc_scan(RunnableContext *ctx)
	{
		gc_scan(ctx->m_instructions);
		value_pool.mark(&ctx->m_localvars);
		size_t i;
		for (i = 0; i < ctx->m_stack_pos; i++)
		{
			gc_scan(ctx->m_data[i]);
		}
	}
	inline void gc_scan(RunningContext *ctx)
	{
		gc_scan(ctx->m_instructions);
		if (ctx->m_caller)
			value_pool.mark(&ctx->m_caller);
		value_pool.mark(&ctx->m_localvars);
		size_t i;
		for (i = 0; i < ctx->m_stack_pos; i++)
		{
			gc_scan(ctx->m_data[i]);
		}
	}

	inline void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Value &msg);
	inline void number_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	inline void float_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	inline void string_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	inline void tuple_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	inline void message_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Value &msg);
	inline void channel_send(Environment *env, const Value &channel, const Value &val, const Value &ret)
	{
		switch (type(channel))
		{
		case RUNNING_CONTEXT: {
			RunningContext *ctx = ptr<RunningContext>(channel);

			ctx->push(val);
			ctx->push(ret);
			env->run(ctx);
			}
			break;
		case RUNNABLE_CONTEXT: {
			RunningContext *ctx = activate_context(env->context(), channel);

			ctx->push(val);
			ctx->push(ret);
			env->run(ctx);
			}
			break;
		case CALLABLE_CONTEXT:
			ptr<CallableContext>(channel)->send(env, val, ret);
			break;
		case NIL: {
			const Value &def = env->special_channel("Channel9::Primitive::NilC");
			return forward_primitive_call(env, def, ret, channel, val);
			}
		case UNDEF: {
			const Value &def = env->special_channel("Channel9::Primitive::UndefC");
			return forward_primitive_call(env, def, ret, channel, val);
			}
		case BFALSE: {
			const Value &def = env->special_channel("Channel9::Primitive::FalseC");
			return forward_primitive_call(env, def, ret, channel, val);
			}
		case BTRUE: {
			const Value &def = env->special_channel("Channel9::Primitive::TrueC");
			return forward_primitive_call(env, def, ret, channel, val);
			}
		case MESSAGE:
			message_channel_simple(env, ret, channel, val);
			break;
		case POSITIVE_NUMBER:
		case NEGATIVE_NUMBER:
			number_channel_simple(env, ret, channel, val);
			break;
		case FLOAT_NUM:
			float_channel_simple(env, ret, channel, val);
			break;
		case STRING:
			string_channel_simple(env, ret, channel, val);
			break;
		case TUPLE:
			tuple_channel_simple(env, ret, channel, val);
			break;
		default:
			printf("Built-in Channel for %i not yet implemented.\n", type(channel));
			exit(1);
		}
	}
}

#include "primitive.hpp"

