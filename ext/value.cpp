#include "value.hpp"
#include "message.hpp"

#include <sstream>

namespace Channel9
{
	MemoryPool value_pool(8*1024*1024);

	Value Value::Nil = {{0}, 0};
	Value Value::True = {{0}, BTRUE};
	Value Value::False = {{0}, BFALSE};
	Value Value::Undef = {{0}, UNDEF};
	Value Value::Zero = {{0}, MACHINE_NUM};
	Value Value::One = {{1}, MACHINE_NUM};
	Value Value::ZTuple = value(&Channel9::ZTuple);
	Value Value::ZString = value(&Channel9::ZString);

	bool complex_compare(const Value &l, const Value &r)
	{
		switch (l.m_type)
		{
		case STRING:
			return *l.str == *r.str;
		case TUPLE:
			return *l.tuple == *r.tuple;
		default:
			return false; // all others compare memory-wise.
		}
	}

	std::string inner_inspect(const Value &val)
	{
		std::stringstream res;
		switch (val.m_type)
		{
		case NIL: return "nil";
		case UNDEF: return "undef";
		case BFALSE: return "false";
		case BTRUE: return "true";
		case MACHINE_NUM: {
			res << "num:";
			res << val.machine_num;
			break;
		}
		case FLOAT_NUM: {
			res << "float:";
			res << val.float_num;
			break;
		}
		case STRING: {
			res << "str:'";
			if (val.str->length() > 128)
				res << val.str->substr(0, 128) << "...";
			else
				res << *val.str;
			res << "'";
			break;
		}
		case TUPLE: {
			res << "tuple[" << val.tuple->size() << "]:(";
			Tuple::const_iterator it;
			int count = 0;
			for (it = val.tuple->begin(); it != val.tuple->end(); ++it)
			{
				if (count < 5)
				{
					res << (count==0?"":", ") << inner_inspect(*it);
				} else {
					res << "...";
					break;
				}
				++count;
			}
			res << ")";
			break;
		}
		case MESSAGE: {
			res << "message:[";
			Message::const_iterator it;
			for (it = val.msg->sysargs(); it != val.msg->sysargs_end(); it++)
				res << inner_inspect(*it) << ",";
			
			res << "]." << *val.msg->name() << "(";

			for (it = val.msg->args(); it != val.msg->args_end(); it++)
				res << inner_inspect(*it) << ",";

			res << ")";
			break;
		}
		case CALLABLE_CONTEXT:
			res << "call_ctx:" << val.call_ctx;
			break;
		case RUNNABLE_CONTEXT:
			res << "ret_ctx:" << val.ret_ctx;
			break;
		}
		return res.str();
	}
	std::string inspect(const Value &val)
	{
		return std::string("<") + inner_inspect(val) + ">";
	}

	void GCRef::scan()
	{
		gc_mark(m_val);
	}
}