#pragma once

#include <vector>

#include "channel9.hpp"
#include "memory_pool.hpp"
#include "string.hpp"

namespace Channel9
{
	union Value
	{
		enum {
			TYPE_SHIFT		= 60,
			TYPE_MASK 		= 0xf000000000000000ULL,
			VALUE_MASK 		= 0x0fffffffffffffffULL,
			POINTER_MASK 	= 0x00007fffffffffffULL,
		};

		uint64_t raw;
		int64_t machine_num;
		double float_num;

		const String *str_p;
		const Tuple *tuple_p;
		const Message *msg_p;
		CallableContext *call_ctx_p;
		RunnableContext *ret_ctx_p;
	};

	extern Value Nil;
	extern Value True;
	extern Value False;
	extern Value Undef;

	inline uint64_t type_mask(ValueType type)
	{
		return (uint64_t)(type) << Value::TYPE_SHIFT;
	}

	inline ValueType basic_type(const Value &val)
	{
		return ValueType((val.raw & Value::TYPE_MASK) >> Value::TYPE_SHIFT);
	}

	inline MemoryPool::Data *header_ptr(const Value &val)
	{
		assert(basic_type(val) == HEAP_TYPE);
		return (MemoryPool::Data*)(val.raw & Value::VALUE_MASK) - 1;
	}

	inline ValueType type(const Value &val)
	{
		ValueType t = basic_type(val);
		if (t != HEAP_TYPE)
			return t;
		else
			return ValueType(header_ptr(val)->m_type);
	}
	inline bool is(const Value &val, ValueType t)
	{
		return type(val) == t;
	}
	inline bool is_truthy(const Value &val) { return !((basic_type(val) & FALSY_MASK) == FALSY_PATTERN); }
	inline bool is_number(const Value &val) { ValueType t = basic_type(val); return t == POSITIVE_NUMBER || t == NEGATIVE_NUMBER; }
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
	tPtr *ptr(const Value &val)
	{
		DO_DEBUG if (!is(val, CALLABLE_CONTEXT)) value_pool.validate((tPtr*)(val.raw & Value::POINTER_MASK));
		return (tPtr*)(val.raw & Value::POINTER_MASK);
	}

	template <typename tPtr>
	inline Value make_value_ptr(ValueType type, tPtr value)
	{
		Value val = {type_mask(type) | ((uint64_t)value & Value::VALUE_MASK)};
		return val;
	}
	inline Value value(long long machine_num)
	{//cut off the top 4 bits, maintaining the sign
		uint64_t type = (uint64_t)(machine_num >> 4) & Value::TYPE_MASK;
		uint64_t num = type | ((uint64_t)(machine_num) & Value::VALUE_MASK);
		Value val = {((uint64_t)(num))};
		return val;
	}
	inline Value value(double float_num)
	{//cut off the bottom 4 bits of precision
		uint64_t num = type_mask(FLOAT_NUM) | (((uint64_t)float_num) >> 4);
		Value val = {((uint64_t)(num))};
		return val;
	}
	inline double float_num(Value val)
	{
		return (double)(val.raw << 4);
	}

	inline Value value(const std::string &str) { return make_value_ptr(STRING, new_string(str)); }
	inline Value value(const String *str) { return make_value_ptr(STRING, str); }

	inline Value value(CallableContext *call_ctx) { return make_value_ptr(CALLABLE_CONTEXT, call_ctx); }
	inline Value value(RunnableContext *ret_ctx) { return make_value_ptr(RUNNABLE_CONTEXT, ret_ctx); }

	void gc_reallocate(Value *from);
	void gc_scan(const Value &from);

	std::string inspect(const Value &val);

	template <typename tVal>
	class GCRef : private GCRoot
	{
	private:
		tVal m_val;

	public:
		GCRef(const tVal &val)
		 : GCRoot(value_pool), m_val(val)
		{}
		GCRef(const GCRef<tVal> &ref)
		 : GCRoot(value_pool), m_val(ref.m_val)
		{}

		void scan()
		{
			gc_reallocate(&m_val);
		}

		const tVal &operator*() const { return m_val; }
		tVal &operator*() { return m_val; }
		const tVal &operator->() const { return m_val; }
		tVal &operator->() { return m_val; }
	};

	template <typename tVal>
	inline GCRef<tVal> gc_ref(const tVal &val)
	{
		return GCRef<tVal>(val);
	}
}
#include "tuple.hpp"

