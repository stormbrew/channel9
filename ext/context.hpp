#pragma once

#include "channel9.hpp"
#include "environment.hpp"
#include "istream.hpp"
#include "memory_pool.hpp"

#include <assert.h>
#include <stdio.h>

namespace Channel9
{
	class VariableFrame
	{
	private:
		std::vector<Value> m_locals;
		VariableFrame *m_parent_frame;

	protected:
		VariableFrame *parent() { return m_parent_frame; }
		const VariableFrame *parent() const { return m_parent_frame; }
		VariableFrame *parent_depth(size_t depth);
		const VariableFrame *parent_depth(size_t depth) const;

	public:
		VariableFrame(size_t local_count, VariableFrame *parent = NULL)
		 : m_locals(local_count, Undef), m_parent_frame(parent)
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

	inline RunnableContext *activate_context(const RunnableContext &copy);

	struct RunnableContext
	{
		IStream *m_instructions;
		const Instruction *m_pos;
		VariableFrame *m_localvars;
		Value *m_sp;

		Value m_data[0];

		void send(Environment *cenv, const Value &val, const Value &ret)
		{
			if (m_sp)
			{
				push(val);
				push(ret);
				cenv->run(this);
			} else {
				RunnableContext *nctx = activate_context(*this);
				nctx->send(cenv, val, ret);
			}
		}

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
		const Value *stack_pos() const { return m_sp; }
		size_t stack_count() const { return m_sp - stack_begin(); }
		void push(const Value &val)
		{
			*m_sp++ = val;
		}
		void pop()
		{
			--m_sp;
		}
		const Value &top() const { return *(m_sp-1); }

		const Value &get_framevar(size_t id) const { return m_data[id]; }
		const Value &get_localvar(size_t id) const { return m_localvars->lookup(id); }
		const Value &get_localvar(size_t id, size_t depth) const { return m_localvars->lookup(id, depth); }

		void set_framevar(size_t id, const Value &val) { m_data[id] = val; }
		void set_localvar(size_t id, const Value &val) { m_localvars->set(id, val); }
		void set_localvar(size_t id, size_t depth, const Value &val) { m_localvars->set(id, depth, val); }

		void new_scope(bool linked = false);
	};

	inline RunnableContext *new_context(IStream *instructions, VariableFrame *localvars = NULL, size_t pos = 0)
	{
		RunnableContext *ctx = (RunnableContext*)malloc(sizeof(RunnableContext) + sizeof(Value)*instructions->frame_count());
		ctx->m_instructions = instructions;
		ctx->m_pos = &*instructions->begin();
		ctx->m_localvars = localvars? localvars : new VariableFrame(instructions->local_count());
		ctx->m_sp = NULL;
		memset(ctx->m_data, 0, sizeof(Value)*instructions->frame_count());
		return ctx;
	}
	inline RunnableContext *new_context(const RunnableContext &copy)
	{
		size_t frame_count = copy.m_instructions->frame_count();
		RunnableContext *ctx = (RunnableContext*)malloc(sizeof(RunnableContext) +
			sizeof(Value)*(frame_count));
		memcpy(ctx, &copy, sizeof(RunnableContext) + sizeof(Value)*frame_count);
		ctx->m_sp = NULL;
		return ctx;
	}
	inline RunnableContext *activate_context(const RunnableContext &copy)
	{
		size_t frame_count = copy.m_instructions->frame_count();
		size_t frame_extra = sizeof(Value)*(frame_count + copy.m_instructions->stack_size());
		RunnableContext *ctx = value_pool.alloc<RunnableContext>(frame_extra);

		memcpy(ctx, &copy, sizeof(RunnableContext) + sizeof(Value)*frame_count);
		ctx->m_sp = ctx->m_data + frame_count;
		return ctx;
	}

	RunnableContext *gc_mark(const RunnableContext *ctx);

	inline void forward_primitive_call(Environment *cenv, const Value &prim_class, const Value &ctx, const Value &oself, const Message &msg);
	inline void number_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg);
	inline void string_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg);
	inline void tuple_channel_simple(Environment *cenv, const Value &ctx, const Value &oself, const Message &msg);
	inline void channel_send(Environment *env, const Value &channel, const Value &val, const Value &ret)
	{
		switch (type(channel))
		{
		case RUNNABLE_CONTEXT:
			ptr<RunnableContext>(channel)->send(env, val, ret);
			break;
		case CALLABLE_CONTEXT:
			ptr<CallableContext>(channel)->send(env, val, ret);
			break;
		case NIL: {
			const Value &def = env->special_channel("Channel9::Primitive::NilC");
			return forward_primitive_call(env, def, ret, channel, *ptr<Message>(val));
			}
		case UNDEF: {
			const Value &def = env->special_channel("Channel9::Primitive::UndefC");
			return forward_primitive_call(env, def, ret, channel, *ptr<Message>(val));
			}
		case BFALSE: {
			const Value &def = env->special_channel("Channel9::Primitive::TrueC");
			return forward_primitive_call(env, def, ret, channel, *ptr<Message>(val));
			}
		case BTRUE: {
			const Value &def = env->special_channel("Channel9::Primitive::FalseC");
			return forward_primitive_call(env, def, ret, channel, *ptr<Message>(val));
			}
		case MESSAGE: {
			const Value &def = env->special_channel("Channel9::Primitive::Message");
			return forward_primitive_call(env, def, ret, channel, *ptr<Message>(val));
			}
		case POSITIVE_NUMBER:
		case NEGATIVE_NUMBER:
			number_channel_simple(env, ret, channel, *ptr<Message>(val));
			break;
		case STRING:
			string_channel_simple(env, ret, channel, *ptr<Message>(val));
			break;
		case TUPLE:
			tuple_channel_simple(env, ret, channel, *ptr<Message>(val));
			break;
		default:
			printf("Built-in Channel for %llu not yet implemented.\n", type(channel) >> 60);
			exit(1);
		}
	}
}

#include "primitive.hpp"

