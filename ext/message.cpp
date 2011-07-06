#include "message.hpp"

namespace Channel9
{
		Message::Message(const std::string &name, const Value::vector &sysargs, const Value::vector &args)
		 : m_name(name), m_sysargs(sysargs), m_args(args)
		{}

		void Message::add_args(const Value::vector &args)
		{
			m_args.insert(m_args.end(), args.begin(), args.end());
		}
		void Message::add_arg(const Value &val)
		{
			m_args.push_back(val);
		}
}