#pragma once

#include "channel9.hpp"
#include "environment.hpp"

#include <assert.h>
#include <stdio.h>
#include <vector>

namespace Channel9
{
	class CallableContext
	{
	private:
		typedef std::vector<CallableContext*> context_set;
		static context_set contexts;
		static bool sweep_flag;

		bool m_sweep;
	
	public:
		CallableContext() : m_sweep(sweep_flag) 
		{ contexts.push_back(this); }

		static void sweep();

		virtual void send(Environment *env, const Value &val, const Value &ret) = 0;
		virtual void scan(); // Make SURE to call down to this.
		virtual std::string inspect() const = 0;
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
}