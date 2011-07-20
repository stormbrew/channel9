#include "value.hpp"
#include "string.hpp"
#include "tuple.hpp"
#include "message.hpp"
#include "context.hpp"

#include <sstream>

namespace Channel9
{
	MemoryPool<8*1024*1024> value_pool;

	Value Nil = {NIL};
	Value True = {BTRUE};
	Value False = {BFALSE};
	Value Undef = {UNDEF};

	bool complex_compare(const Value &l, const Value &r)
	{
		switch (type(l))
		{
		case STRING:
			return *ptr<String>(l) == *ptr<String>(r);
		case TUPLE:
			return *ptr<Tuple>(l) == *ptr<Tuple>(r);
		default:
			return false; // all others compare memory-wise.
		}
	}

	inline void gc_reallocate(Value *from)
	{
		switch (type(*from))
		{
		case STRING: {
			String *str = ptr<String>(*from);
			gc_reallocate(&str);
			*from = make_value_ptr(STRING, str);
			break;
		}
		case TUPLE: {
			Tuple *tuple = ptr<Tuple>(*from);
			gc_reallocate(&tuple);
			*from = make_value_ptr(TUPLE, tuple);
		}
		case MESSAGE: {
			Message *msg = ptr<Message>(*from);
			gc_reallocate(&msg);
			*from = make_value_ptr(MESSAGE, msg);
		}
		case RUNNABLE_CONTEXT: {
			RunnableContext *ctx = ptr<RunnableContext>(*from);
			gc_reallocate(&ctx);
			*from = make_value_ptr(RUNNABLE_CONTEXT, ctx);
		}
		default: break;
		}
	}
	inline void gc_scan(const Value &from)
	{
		switch (type(from))
		{
		case STRING:
			return gc_scan(ptr<String>(from));
		case TUPLE:
			return gc_scan(ptr<Tuple>(from));
		case MESSAGE:
			return gc_scan(ptr<Message>(from));
		case CALLABLE_CONTEXT:
			return gc_scan(ptr<CallableContext>(from));
		case RUNNABLE_CONTEXT:
			return gc_scan(ptr<RunnableContext>(from));
		default: break;
		}
	}

	std::string inner_inspect(const Value &val)
	{
		std::stringstream res;
		switch (type(val))
		{
		case NIL: return "nil";
		case UNDEF: return "undef";
		case BFALSE: return "false";
		case BTRUE: return "true";
		case POSITIVE_NUMBER:
		case NEGATIVE_NUMBER: {
			res << "num:";
			res << val.machine_num;
			break;
		}
//		case FLOAT_NUM: {
//			res << "float:";
//			res << val.float_num;
//			break;
//		}
		case STRING: {
			res << "str:'";
			if (ptr<String>(val)->length() > 128)
				res << ptr<String>(val)->substr(0, 128) << "...";
			else
				res << *ptr<String>(val);
			res << "'";
			break;
		}
		case TUPLE: {
			res << "tuple[" << ptr<Tuple>(val)->size() << "]:(";
			Tuple::const_iterator it;
			int count = 0;
			for (it = ptr<Tuple>(val)->begin(); it != ptr<Tuple>(val)->end(); ++it)
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
			for (it = ptr<Message>(val)->sysargs(); it != ptr<Message>(val)->sysargs_end(); it++)
				res << inner_inspect(*it) << ",";
			
			res << "]." << *ptr<Message>(val)->name() << "(";

			for (it = ptr<Message>(val)->args(); it != ptr<Message>(val)->args_end(); it++)
				res << inner_inspect(*it) << ",";

			res << ")";
			break;
		}
		case CALLABLE_CONTEXT:
			res << "call_ctx:" << ptr<CallableContext>(val);
			break;
		case RUNNABLE_CONTEXT:
			res << "ret_ctx:" << ptr<RunnableContext>(val);
			break;
		}
		return res.str();
	}
	std::string inspect(const Value &val)
	{
		return std::string("<") + inner_inspect(val) + ">";
	}
}