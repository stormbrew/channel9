#include "istream.hpp"

namespace Channel9
{
	void IStream::add(const Instruction &instruction)
	{
		switch (instruction.instruction)
		{
		case LOCAL_SET:
		case LOCAL_GET:
			local(*instruction.arg2.str);
			break;
		case FRAME_GET:
		case FRAME_SET:
			frame(*instruction.arg1.str);
			break;
		default:
			break;
		}
		m_instructions.push_back(instruction);
	}
	void IStream::set_label(const std::string &label)
	{
		m_labels[label] = m_instructions.size();
	}
	void IStream::set_source_pos(const SourcePos &sp)
	{
		m_source_positions[m_instructions.size()] = sp;
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