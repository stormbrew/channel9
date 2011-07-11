#include "value.hpp"
#include "message.hpp"

#include <sstream>

namespace Channel9
{
	Value Value::Nil = {{0}, 0};
	Value Value::True = {{0}, BTRUE};
	Value Value::False = {{0}, BFALSE};
	Value Value::Undef = {{0}, UNDEF};
	Value Value::Zero = {{0}, MACHINE_NUM};
	Value Value::One = {{1}, MACHINE_NUM};
	Value Value::ZTuple = value(Value::vector());
	Value Value::ZString = value(std::string());

#	define MAKE_VALUE(type, value) { \
		Value val = {{value}, (type)}; \
		return val; \
	}
#	define MAKE_VALUE_PTR(type, key, value) { \
		Value val = {{0}, (type)}; \
		val.key = (value); \
		return val; \
	}
	Value value(long long machine_num) MAKE_VALUE(MACHINE_NUM, machine_num);
	Value value(double float_num) MAKE_VALUE(FLOAT_NUM, float_num);

	Value value(const std::string &str) MAKE_VALUE_PTR(STRING, str, new std::string(str));
	Value value(const std::vector<Value> &tuple) MAKE_VALUE_PTR(TUPLE, tuple, new std::vector<Value>(tuple));
	Value value(const Message &msg) MAKE_VALUE_PTR(MESSAGE, msg, new Message(msg));

	Value value(CallableContext *call_ctx) MAKE_VALUE_PTR(CALLABLE_CONTEXT, call_ctx, call_ctx);
	Value value(RunnableContext *ret_ctx) MAKE_VALUE_PTR(RUNNABLE_CONTEXT, ret_ctx, ret_ctx);

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
			Value::vector::const_iterator it;
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
			Value::vector::const_iterator it;
			for (it = val.msg->begin_sys(); it != val.msg->end_sys(); it++)
				res << inner_inspect(*it) << ",";
			
			res << "]." << val.msg->name() << "(";

			for (it = val.msg->begin(); it != val.msg->end(); it++)
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
}