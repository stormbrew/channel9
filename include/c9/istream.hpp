#pragma once

#include <map>
#include <vector>
#include <string>
#include <sstream>

#include "c9/channel9.hpp"
#include "c9/instruction.hpp"
#include "c9/callable_context.hpp"
#include "c9/value.hpp"
#include "json/json.h"

namespace Channel9
{
	struct SourcePos
	{
		SourcePos()
		 : line_num(-1), column(-1)
		{}
		SourcePos(const std::string &file, size_t line_num, size_t column, const std::string &annotation)
		 : file(file), line_num(line_num), column(column), annotation(annotation)
		{}

		std::string file;
		size_t line_num;
		size_t column;
		std::string annotation;

		std::string inspect() const
		{
			std::stringstream str;
			str << file << "@" << line_num << "+" << column;
			if (annotation.size() > 0)
				str << " (" << annotation << ")";
			return str.str();
		}

		bool operator==(const SourcePos &o)
		{
			return line_num == o.line_num && column == o.column && file == o.file;
		}
		bool operator!=(const SourcePos &o)
		{
			return !operator==(o);
		}
	};

	class IStream : public CallableContext
	{
	private:
		std::map<std::string, size_t> m_labels;
		std::vector<SourcePos> m_source_info;

		typedef std::map<std::string, size_t> name_map;
		name_map m_lexicals;
		name_map m_frames;

		std::vector<Instruction> m_instructions;
		std::vector<size_t> m_source_positions;
		std::size_t m_cur_source_pos;

		// only valid after call to normalize
		size_t m_stack_size;
		size_t m_local_size;
		size_t m_stack_offset;
		size_t m_local_offset;
		size_t m_frame_size;

		typedef std::pair<bool, size_t> pos_info;
		size_t normalize(size_t stack_size, name_map &locals, size_t pos, std::vector<pos_info> &pos_map);

	public:
		void add(const std::string &instruction, const Value &arg1 = Nil, const Value &arg2 = Nil, const Value &arg3 = Nil)
		{ add(inum(instruction), arg1, arg2, arg3); }
		void add(INUM ins, const Value &arg1 = Nil, const Value &arg2 = Nil, const Value &arg3 = Nil)
		{ add(instruction(ins, arg1, arg2, arg3)); }
		void add(Instruction instruction);
		void set_label(const std::string &label);
		void set_source_pos(const SourcePos &sp);
		SourcePos &source_pos(size_t ipos);
		SourcePos &source_pos(const Instruction *ipos) { return source_pos(ipos - &*m_instructions.begin()); }

		size_t label_pos(const std::string &label) const { return m_labels.find(label)->second; }

		size_t lexical(const std::string &name);
		size_t lexical(const std::string &name) const;
		size_t local(const std::string &name);
		size_t local(const std::string &name) const;
		size_t frame(const std::string &name);
		size_t frame(const std::string &name) const;
		size_t lexical_count() const;
		size_t frame_count() const;

		size_t stack_size() const { return m_stack_size; }
		size_t stack_offset() const { return m_stack_offset; }
		size_t local_offset() const { return m_local_offset; }
		size_t frame_size() const { return m_frame_size; }

		// Turn jmp label references into numeric references.
		// Returns the maximum stack size.
		size_t normalize();
		Json::Value to_json() const;

		typedef std::vector<Instruction> ivector;
		typedef ivector::iterator iterator;
		typedef ivector::const_iterator const_iterator;

		iterator begin() { return m_instructions.begin(); }
		iterator end() { return m_instructions.end(); }
		const_iterator begin() const { return m_instructions.begin(); }
		const_iterator end() const { return m_instructions.end(); }

		void scan()
		{
			for (IStream::iterator it = begin(); it != end(); it++)
			{
				gc_scan(NULL, &*it);
			}
			CallableContext::scan();
		}
		void send(Environment *env, const Value &val, const Value &ret);

		std::string inspect() const
		{
			return "IStream";
		}
	};
	// Specialize GCRef's scanner to only do a gc_scan of it.
	template <>
	inline void GCRef<IStream*>::scan()
	{
		gc_scan(NULL, m_val);
	}
}

