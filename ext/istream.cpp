#include "istream.hpp"
#include <assert.h>
#include <stdio.h>

namespace Channel9
{
	size_t IStream::normalize(size_t stack_size, size_t pos, std::vector<pos_info> &pos_map)
	{
		size_t max_size = stack_size;
		bool done = false;
		while (!done)
		{
			assert(pos < m_instructions.size());
			pos_info &info = pos_map[pos];

			Instruction &ins = m_instructions[pos++];
			InstructionInfo insinfo = iinfo(ins);

			switch (ins.instruction)
			{
			case JMP:
				{
					size_t branch_pos = label_pos(ptr<String>(ins.arg1)->str());
					ins.arg3 = value((long long)branch_pos);
				}
				break;
			case JMP_IF:
			case JMP_IF_NOT:
				{
					size_t branch_pos = label_pos(ptr<String>(ins.arg1)->str());
					ins.arg3 = value((long long)branch_pos);
				}
				break;
			case CHANNEL_NEW:
				{
					size_t branch_pos = label_pos(ptr<String>(ins.arg1)->str());
					ins.arg3 = value((long long)branch_pos);
				}
				break;
			case CHANNEL_RET:
			case CHANNEL_SEND:
				done = true;
				break;
			default:
				break;
			}

			TRACE_OUT(TRACE_VM, TRACE_INFO) {
				SourcePos spos = source_pos(pos-1);
				tprintf("Analyzing bytecode pos %d: %s[%d-%d+%d=%d], max: %d\n  Line Info: %s:%d\n",
					(int)pos-1, inspect(ins).c_str(),
					(int)stack_size, (int)insinfo.in, (int)insinfo.out, (int)(stack_size - insinfo.in + insinfo.out),
					(int)max_size, spos.file.c_str(), (int)spos.line_num);
			}
			assert(stack_size >= insinfo.in);

			stack_size -= insinfo.in;
			stack_size += insinfo.out;

			if (info.first) // visited
			{
				// make sure that the stack size is the same
				// as it was on the previous visit to this node.
				TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Visited position %d before. Stack was %d, is now %d\n", (int)pos, (int)info.second, (int)stack_size);
				assert(info.second == stack_size);
				done = true;
			} else {
				info.first = true;
				info.second = stack_size;

				if (stack_size > max_size) max_size = stack_size;

				switch (ins.instruction)
				{
				case JMP:
					pos = (size_t)ins.arg3.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Unconditional branch to pos %d\n", (int)pos);
					break;
				case JMP_IF:
				case JMP_IF_NOT:
					{
						size_t branch_pos = (size_t)ins.arg3.machine_num;
						TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Conditional branch. Options: %d | %d\n", (int)pos, (int)branch_pos);
						size_t r_max = normalize(stack_size, branch_pos, pos_map);
						if (r_max > max_size) max_size = r_max;
					}
					break;
				case CHANNEL_NEW:
					{
						size_t branch_pos = (size_t)ins.arg3.machine_num;
						TRACE_PRINTF(TRACE_VM, TRACE_INFO, "New channel. Options: %d | %d\n", (int)pos, (int)branch_pos);
						size_t r_max = normalize(2, branch_pos, pos_map);
						if (r_max > max_size) max_size = r_max;
					}
					break;
				case CHANNEL_RET:
				case CHANNEL_SEND:
					done = true;
					break;
				default:
					break;
				}
			}
		}
		return max_size;
	}

	size_t IStream::normalize()
	{
		size_t stack_size = 2; // entry point always starts at 2.
		size_t pos = 0;
		typedef std::pair<bool, size_t> pos_info;
		std::vector<pos_info> pos_map(m_instructions.size(), pos_info(false, 0));

		m_stack_size = normalize(stack_size, pos, pos_map);
		TRACE_PRINTF(TRACE_VM, TRACE_INFO, "Found max stack of stream to be %d\n", (int)m_stack_size);
		return m_stack_size;
	}

	void IStream::add(Instruction instruction)
	{
		long long local_id;
		switch (instruction.instruction)
		{
		case LOCAL_SET:
		case LOCAL_GET:
			local_id = local(ptr<String>(instruction.arg2)->str());
			instruction.arg3 = value(local_id);
			break;
		case FRAME_GET:
		case FRAME_SET:
			local_id = frame(ptr<String>(instruction.arg1)->str());
			instruction.arg3 = value(local_id);
			break;
		default:
			break;
		}
		m_instructions.push_back(instruction);
		m_source_positions.push_back(m_cur_source_pos);
	}
	void IStream::set_label(const std::string &label)
	{
		m_labels[label] = m_instructions.size();
	}
	void IStream::set_source_pos(const SourcePos &sp)
	{
		m_source_info.push_back(sp);
		m_cur_source_pos = m_source_info.size() - 1;
	}
	SourcePos IStream::source_pos(size_t ipos)
	{
		if (ipos < m_source_positions.size())
			return m_source_info[m_source_positions[ipos]];
		else
			return SourcePos();
	}

	size_t IStream::local(const std::string &name)
	{
		name_map::iterator it = m_locals.find(name);
		if (it == m_locals.end())
		{
			size_t num = m_locals.size();
			m_locals[name] = num;
			return num;
		} else {
			return it->second;
		}
	}
	size_t IStream::local(const std::string &name) const
	{
		name_map::const_iterator it = m_locals.find(name);
		if (it == m_locals.end())
		{
			return -1;
		} else {
			return it->second;
		}
	}
	size_t IStream::frame(const std::string &name)
	{
		name_map::iterator it = m_frames.find(name);
		if (it == m_frames.end())
		{
			size_t num = m_frames.size();
			m_frames[name] = num;
			return num;
		} else {
			return it->second;
		}
	}
	size_t IStream::frame(const std::string &name) const
	{
		name_map::const_iterator it = m_frames.find(name);
		if (it == m_frames.end())
		{
			return -1;
		} else {
			return it->second;
		}
	}
	size_t IStream::local_count() const
	{
		return m_locals.size();
	}
	size_t IStream::frame_count() const
	{
		return m_frames.size();
	}
}

