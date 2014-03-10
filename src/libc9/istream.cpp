#include "c9/istream.hpp"
#include "c9/context.hpp"
#include "c9/message.hpp"

#include <assert.h>
#include <stdio.h>

namespace Channel9
{
	size_t IStream::normalize(size_t stack_size, name_map &locals, size_t pos, std::vector<pos_info> &pos_map)
	{
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
					ins.cache = value((long long)branch_pos);
				}
				break;
			case JMP_IF:
			case JMP_IF_NOT:
				{
					size_t branch_pos = label_pos(ptr<String>(ins.arg1)->str());
					ins.cache = value((long long)branch_pos);
				}
				break;
			case CHANNEL_NEW:
				{
					size_t branch_pos = label_pos(ptr<String>(ins.arg1)->str());
					ins.cache = value((long long)branch_pos);
				}
				break;
			case LOCAL_SET:
			case LOCAL_GET:
				{
					std::string name = ptr<String>(ins.arg1)->c_str();
					name_map::iterator it = locals.find(name);
					if (it == locals.end())
					{
						ins.cache = value((long long)locals.size());
						locals.insert(make_pair(name, locals.size()));
						if (m_local_size < locals.size()) m_local_size = locals.size();
					} else {
						ins.cache = value((long long)it->second);
					}
				}
				break;
			case CHANNEL_RET:
			case CHANNEL_SEND:
				done = true;
				break;
			default:
				break;
			}

			TRACE_OUT(TRACE_VM, TRACE_DEBUG) {
				SourcePos spos = source_pos(pos-1);
				tprintf("Analyzing bytecode pos %d: %s[%d-%d+%d=%d], stack_max: %d, local_max: %d\n  Line Info: %s:%d\n",
					(int)pos-1, Channel9::inspect(ins).c_str(),
					(int)stack_size, (int)insinfo.in, (int)insinfo.out, (int)(stack_size - insinfo.in + insinfo.out),
					(int)m_stack_size, (int)m_local_size, spos.file.c_str(), (int)spos.line_num);
			}
			assert(stack_size >= insinfo.in);

			stack_size -= insinfo.in;
			stack_size += insinfo.out;

			if (info.first) // visited
			{
				// make sure that the stack size is the same
				// as it was on the previous visit to this node.
				TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Visited position %d before. Stack was %d, is now %d\n", (int)pos, (int)info.second, (int)stack_size);
				assert(info.second == stack_size);
				done = true;
			} else {
				info.first = true;
				info.second = stack_size;

				if (stack_size > m_stack_size) m_stack_size = stack_size;

				switch (ins.instruction)
				{
				case JMP:
					pos = (size_t)ins.cache.machine_num;
					TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Unconditional branch to pos %d\n", (int)pos);
					break;
				case JMP_IF:
				case JMP_IF_NOT:
					{
						size_t branch_pos = (size_t)ins.cache.machine_num;
						TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Conditional branch. Options: %d | %d\n", (int)pos, (int)branch_pos);
						normalize(stack_size, locals, branch_pos, pos_map);
					}
					break;
				case CHANNEL_NEW:
					{
						size_t branch_pos = (size_t)ins.cache.machine_num;
						TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "New channel. Options: %d | %d\n", (int)pos, (int)branch_pos);
						std::map<std::string, size_t> sublocals;
						normalize(2, sublocals, branch_pos, pos_map);
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
		return m_stack_size;
	}

	size_t IStream::normalize()
	{
		size_t pos = 0;
		typedef std::pair<bool, size_t> pos_info;
		std::vector<pos_info> pos_map(m_instructions.size(), pos_info(false, 0));
		std::map<std::string, size_t> locals;

		m_stack_size = 2;
		m_local_size = 0;
		normalize(2, locals, pos, pos_map);
		m_stack_offset = frame_count() + m_local_size;
		m_local_offset = frame_count();
		m_frame_size = m_stack_size + m_stack_offset;
		TRACE_PRINTF(TRACE_VM, TRACE_DEBUG, "Found max stack of stream to be %d\n", (int)m_stack_size);
		return m_stack_size;
	}

	Json::Value IStream::to_json() const
	{
		Json::Value code(Json::arrayValue);

		Json::Value document(Json::objectValue);
		document["code"] = code;
		return code;
	}

	void IStream::add(Instruction instruction)
	{
		long long id;
		switch (instruction.instruction)
		{
		case LEXICAL_SET:
		case LEXICAL_GET:
			id = lexical(ptr<String>(instruction.arg2)->str());
			instruction.cache = value(id);
			break;
		case FRAME_GET:
		case FRAME_SET:
			id = frame(ptr<String>(instruction.arg1)->str());
			instruction.cache = value(id);
			break;
		case MESSAGE_NEW:
		case MESSAGE_FORWARD:
		case MESSAGE_IS:
		case MESSAGE_IS_PROTO:
			instruction.cache = value((long long)make_message_id(ptr<String>(instruction.arg1)));
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
	SourcePos &IStream::source_pos(size_t ipos)
	{
		static SourcePos empty;
		if (ipos < m_source_positions.size())
			return m_source_info[m_source_positions[ipos]];
		else
			return empty;
	}

	size_t IStream::lexical(const std::string &name)
	{
		name_map::iterator it = m_lexicals.find(name);
		if (it == m_lexicals.end())
		{
			size_t num = m_lexicals.size();
			m_lexicals[name] = num;
			return num;
		} else {
			return it->second;
		}
	}
	size_t IStream::lexical(const std::string &name) const
	{
		name_map::const_iterator it = m_lexicals.find(name);
		if (it == m_lexicals.end())
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
	size_t IStream::lexical_count() const
	{
		return m_lexicals.size();
	}
	size_t IStream::frame_count() const
	{
		return m_frames.size();
	}

	void IStream::send(Environment *env, const Value &val, const Value &ret)
	{
		channel_send(env, ret, Nil, Nil);
	}

}

