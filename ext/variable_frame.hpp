#pragma once

#include "channel9.hpp"
#include "memory_pool.hpp"
#include "value.hpp"
#include "istream.hpp"

#include <vector>

namespace Channel9
{
	struct VariableFrame
	{
		IStream *m_instructions;
		VariableFrame *m_parent_frame;
		Value m_locals[0];

		VariableFrame *parent() { return m_parent_frame; }
		const VariableFrame *parent() const { return m_parent_frame; }
		VariableFrame *parent_depth(size_t depth);
		const VariableFrame *parent_depth(size_t depth) const;

		const Value &lookup(size_t id) const { return m_locals[id]; }
		const Value &lookup(size_t id, size_t depth) const;
		void set(size_t id, const Value &val) { m_locals[id] = val; }
		void set(size_t id, size_t depth, const Value &val);
	};

	inline VariableFrame *new_variable_frame(IStream *instructions, VariableFrame *parent = NULL)
	{
		size_t local_count = instructions->local_count();
		VariableFrame *frame = value_pool.alloc<VariableFrame>(local_count * sizeof(Value));
		frame->m_instructions = instructions;
		frame->m_parent_frame = parent;
		for (size_t i = 0; i < local_count; i++)
		{
			frame->m_locals[i] = Nil;
		}
		return frame;
	}

	inline void gc_reallocate(VariableFrame **from)
	{
		size_t local_count = (*from)->m_instructions->local_count();
		VariableFrame *nframe = value_pool.alloc<VariableFrame>(local_count * sizeof(Value));
		memcpy(nframe, *from, sizeof(VariableFrame) + local_count * sizeof(Value));
		(*from) = nframe;
	}
	inline void gc_scan(VariableFrame *from)
	{
		size_t local_count = from->m_instructions->local_count();
		gc_scan(from->m_instructions);
		gc_reallocate(&from->m_parent_frame);
		for (size_t i = 0; i < local_count; i++)
		{
			gc_reallocate(&from->m_locals[i]);
		}
	}
}