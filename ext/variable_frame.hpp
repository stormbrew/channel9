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
		Value m_lexicals[0];

		void link_frame(VariableFrame *parent) { m_parent_frame = parent; }

		VariableFrame *parent() { return m_parent_frame; }
		const VariableFrame *parent() const { return m_parent_frame; }
		VariableFrame *parent_depth(size_t depth);
		const VariableFrame *parent_depth(size_t depth) const;

		const Value &lookup(size_t id) const { return m_lexicals[id]; }
		const Value &lookup(size_t id, size_t depth) const;
		void set(size_t id, const Value &val) { m_lexicals[id] = val; }
		void set(size_t id, size_t depth, const Value &val);
	};

	inline VariableFrame *new_variable_frame(IStream *instructions)
	{
		size_t lexical_count = instructions->lexical_count();
		VariableFrame *frame = value_pool.alloc<VariableFrame>(lexical_count * sizeof(Value), VARIABLE_FRAME);
		frame->m_instructions = instructions;
		frame->m_parent_frame = NULL;
		for (size_t i = 0; i < lexical_count; i++)
		{
			frame->m_lexicals[i] = Nil;
		}
		return frame;
	}

	inline void gc_scan(VariableFrame *from)
	{
		size_t lexical_count = from->m_instructions->lexical_count();
		gc_scan(from->m_instructions);
		if (from->m_parent_frame)
			value_pool.mark(&from->m_parent_frame);

		for (size_t i = 0; i < lexical_count; i++)
		{
			gc_scan(from->m_lexicals[i]);
		}
	}
}

