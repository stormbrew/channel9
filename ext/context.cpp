#include "context.hpp"
#include "environment.hpp"

namespace Channel9
{
	MemoryPool<8*1024*1024> frame_pool;

	void RunnableContext::jump(const std::string &label)
	{
		m_pos = &*m_instructions->begin() + m_instructions->label_pos(label);
	}
	
	void RunnableContext::new_scope(bool linked)
	{
		m_localvars = new_variable_frame(m_instructions, m_localvars);
	}
}