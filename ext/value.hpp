#pragma once

#include <string>
#include <vector>

#include "channel9.hpp"

namespace Channel9
{
	enum ValueType
	{
		NIL = 0,
		UNDEF,
		BFALSE,
		BTRUE,
		MACHINE_NUM,
		FLOAT_NUM,
		STRING,
		TUPLE,
		MESSAGE,
		CALLABLE_CONTEXT,
		RUNNABLE_CONTEXT,
	};

	struct Value
	{
		union {
			long long machine_num;
			double float_num;
			const std::string *str;
			const std::vector<Value> *tuple;
			const Message *msg;
			CallableContext *call_ctx;
			RunnableContext *ret_ctx;
		};
		char m_type;

		static Value Nil;
		static Value True;
		static Value False;
		static Value Undef;
		static Value Zero;
		static Value One;
		static Value ZTuple;
		static Value ZString;
		
		typedef std::vector<Value> vector;
	} __attribute__((__packed__));

	bool complex_compare(const Value &l, const Value &r);

	inline bool operator==(const Value &l, const Value &r)
	{
		if (memcmp(&l, &r, sizeof(Value)) == 0)
			return true;
		else if (l.m_type == r.m_type) {
			return complex_compare(l, r);
		} else {
			return false;
		}
	}
	inline bool operator!=(const Value &l, const Value &r)
	{
		return !(l == r);
	}

	inline bool is_truthy(const Value &val) { return val.m_type >= BTRUE; }

	inline const Value &bvalue(bool truthy) { return truthy ? Value::True : Value::False; }
	Value value(long long machine_num);
	Value value(double float_num);
	Value value(const std::string &str);
	Value value(const std::vector<Value> &tuple);
	Value value(const Message &msg);
	Value value(CallableContext *call_ctx);
	Value value(RunnableContext *ret_ctx);

	std::string inspect(const Value &val);
}