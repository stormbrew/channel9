#pragma once

#include <map>
#include <vector>
#include <string>

#include "channel9.hpp"
#include "instruction.hpp"
#include "value.hpp"

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
	};

	class IStream
	{
	private:
		std::map<std::string, size_t> m_labels;
		std::map<size_t, SourcePos> m_source_positions;

		typedef std::map<std::string, size_t> name_map;
		name_map m_locals;
		name_map m_frames;

		std::vector<Instruction> m_instructions;

	public:
		void add(const std::string &instruction, const Value &arg1 = Value::Nil, const Value &arg2 = Value::Nil, const Value &arg3 = Value::Nil)
		{ add(inum(instruction), arg1, arg2, arg3); }
		void add(INUM ins, const Value &arg1 = Value::Nil, const Value &arg2 = Value::Nil, const Value &arg3 = Value::Nil)
		{ add(instruction(ins, arg1, arg2, arg3)); }
		void add(const Instruction &instruction);
		void set_label(const std::string &label);
		void set_source_pos(const SourcePos &sp);

		size_t label_pos(const std::string &label) const { return m_labels.find(label)->second; }

		size_t local(const std::string &name);
		size_t local(const std::string &name) const;
		size_t frame(const std::string &name);
		size_t frame(const std::string &name) const;
		size_t local_count() const;
		size_t frame_count() const;

		typedef std::vector<Instruction> ivector;
		typedef ivector::iterator iterator;
		typedef ivector::const_iterator const_iterator;

		iterator begin() { return m_instructions.begin(); }
		iterator end() { return m_instructions.end(); }
		const_iterator begin() const { return m_instructions.begin(); }
		const_iterator end() const { return m_instructions.end(); }
	};
}