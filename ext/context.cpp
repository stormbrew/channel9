#include "context.hpp"
#include "environment.hpp"

namespace Channel9
{
	void RunningContext::jump(const std::string &label)
	{
		m_pos = &*m_instructions->begin() + m_instructions->label_pos(label);
	}
}