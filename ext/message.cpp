#include "message.hpp"

namespace Channel9
{
		void Message::add_args(const Value::vector &args)
		{
			m_args.insert(m_args.end(), args.begin(), args.end());
		}
		void Message::add_arg(const Value &val)
		{
			m_args.push_back(val);
		}
		void Message::prefix_args(const Value::vector &args)
		{
			m_args.insert(m_args.begin(), args.begin(), args.end());
		}
		void Message::prefix_arg(const Value &val)
		{
			m_args.insert(m_args.begin(), val);
		}
}