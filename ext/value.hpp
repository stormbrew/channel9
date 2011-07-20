#pragma once

#include <vector>

#include "channel9.hpp"
#include "string.hpp"
#include "memory_pool.hpp"

namespace Channel9
{
	enum
	{
		POSITIVE_NUMBER = 0x0000000000000000ULL,

		BTRUE 			= 0x1000000000000000ULL,
		FLOAT_NUM 		= 0x2000000000000000ULL,
		STRING 			= 0x3000000000000000ULL,
		TUPLE 			= 0x4000000000000000ULL,
		MESSAGE 		= 0x5000000000000000ULL,
		CALLABLE_CONTEXT= 0x6000000000000000ULL,
		RUNNABLE_CONTEXT= 0x7000000000000000ULL,

		// top two bits in pattern 10 indicates falsy,
		// all others are truthy.
		FALSY_MASK 		= 0xc000000000000000ULL,
		FALSY_PATTERN	= 0x8000000000000000ULL,

		NIL 			= 0x8000000000000000ULL,
		UNDEF 			= 0x9000000000000000ULL,
		BFALSE 			= 0xA000000000000000ULL,
		// don't use    = 0xB000000000000000ULL,

		NEGATIVE_NUMBER = 0xf000000000000000ULL,

		TYPE_MASK 		= 0xf000000000000000ULL,
		VALUE_MASK 		= 0x0fffffffffffffffULL,
	};
	typedef unsigned long long ValueType;

	union Value
	{
		unsigned long long raw;
		long long machine_num;
//		double float_num; // commented out until I can get this done properly.

		const String *str_p;
		const Tuple *tuple_p;
		const Message *msg_p;
		CallableContext *call_ctx_p;
		RunnableContext *ret_ctx_p;
	};

	template <typename tPtr>
	tPtr *ptr(const Value &val)
	{
		return (tPtr*)(val.raw & VALUE_MASK);
	}

	extern Value Nil;
	extern Value True;
	extern Value False;
	extern Value Undef;

	inline ValueType type(const Value &val)
	{
		return ValueType(val.raw & TYPE_MASK);
	}
	inline bool is(const Value &val, unsigned long long t)
	{
		return type(val) == t;
	}
	inline bool is_truthy(const Value &val) { return !((val.raw & FALSY_MASK) == FALSY_PATTERN); }
	inline bool is_number(const Value &val) { ValueType t = type(val); return t == POSITIVE_NUMBER || t == NEGATIVE_NUMBER; }
	inline bool same_type(const Value &l, const Value &r)
	{
		return type(l) == type(r);
	}

	bool complex_compare(const Value &l, const Value &r);

	inline bool operator==(const Value &l, const Value &r)
	{
		if (memcmp(&l, &r, sizeof(Value)) == 0)
			return true;
		else if (same_type(l, r)) {
			return complex_compare(l, r);
		} else {
			return false;
		}
	}
	inline bool operator!=(const Value &l, const Value &r)
	{
		return !(l == r);
	}

	inline const Value &bvalue(bool truthy) { return truthy ? True : False; }

	template <typename tPtr>
	inline Value make_value_ptr(ValueType type, tPtr value)
	{
		DO_DEBUG assert((unsigned long long)value < TYPE_MASK);
		Value val = {(type & TYPE_MASK) | ((unsigned long long)value & VALUE_MASK)};
		return val;
	}
	inline Value value(long long machine_num)
	{
		unsigned long long type = (unsigned long long)(machine_num >> 4) & TYPE_MASK;
		unsigned long long num = type | ((unsigned long long)(machine_num) & VALUE_MASK);
		Value val = {((unsigned long long)(num))};
		return val;
	}
//	inline Value value(double float_num) MAKE_VALUE(FLOAT_NUM, float_num);

	inline Value value(const std::string &str) { return make_value_ptr(STRING, new_string(str)); }
	inline Value value(const String *str) { return make_value_ptr(STRING, str); }

	inline Value value(CallableContext *call_ctx) { return make_value_ptr(CALLABLE_CONTEXT, call_ctx); }
	inline Value value(RunnableContext *ret_ctx) { return make_value_ptr(RUNNABLE_CONTEXT, ret_ctx); }

	void gc_reallocate(Value *from);
	void gc_scan(const Value &from);

	std::string inspect(const Value &val);
}
#include "tuple.hpp"

