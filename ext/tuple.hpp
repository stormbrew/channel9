#pragma once

#include "memory_pool.hpp"

#include <vector>
#include <algorithm>
#include <assert.h>

namespace Channel9
{
	struct Tuple
	{
		size_t m_count;
		Value m_data[0];

		size_t count() const { return m_count; }
		size_t size() const { return m_count; }
		size_t length() const { return m_count; }

		typedef Value *iterator;
		typedef const Value *const_iterator;

		iterator begin() { return m_data; }
		iterator end() { return m_data + m_count; }
		const_iterator begin() const { return m_data; }
		const_iterator end() const { return m_data + m_count; }

		const Value &operator[](size_t idx) const { return m_data[idx]; }

		std::vector<Value> vector() const
		{
			return std::vector<Value>(m_data, m_data + m_count);
		}

		bool equal(const Tuple &r) const
		{
			return this == &r || (m_count == r.m_count && std::equal(m_data, m_data + m_count, r.m_data));
		}
		bool equal(const std::vector<Value> &vec) const
		{ 
			return m_count == vec.size() && std::equal(vec.begin(), vec.end(), m_data); 
		}
	};

	inline bool operator==(const Tuple &l, const Tuple &r)
	{
		return l.equal(r);
	}
	inline bool operator!=(const Tuple &l, const Tuple &r)
	{
		return !l.equal(r);
	}
	template <typename tOther>
	inline bool operator==(const Tuple &l, const tOther &r)
	{
		return l.equal(r);
	}
	template <typename tOther>
	inline bool operator!=(const Tuple &l, const tOther &r)
	{
		return !l.equal(r);
	}
	template <typename tOther>
	inline bool operator==(const tOther &r, const Tuple &l)
	{
		return l.equal(r);
	}
	template <typename tOther>
	inline bool operator!=(const tOther &r, const Tuple &l)
	{
		return !l.equal(r);
	}

	inline Tuple *new_tuple(size_t len)
	{
		Tuple *ret = value_pool.alloc<Tuple>(sizeof(Value)*len);
		ret->m_count = len;
		
		return ret;
	}
	template <typename tIter>
	inline Tuple *new_tuple(size_t len, tIter str)
	{
		Tuple *ret = new_tuple(len);
		std::copy(str, str + len, ret->m_data);
		return ret;
	}
	template <typename tIter>
	inline Tuple *new_tuple(tIter beg, tIter end)
	{
		Tuple *ret = new_tuple(end - beg);
		std::copy(beg, end, ret->m_data);
		return ret;
	}
	inline Tuple *new_tuple(const std::vector<Value> &str)
	{
		return new_tuple(str.begin(), str.end());
	}

	inline void gc_reallocate(Tuple **from)
	{
		Tuple *ntuple = value_pool.alloc<Tuple>((*from)->m_count * sizeof(Value));
		memcpy(ntuple, *from, sizeof(Tuple) + (*from)->m_count * sizeof(Value));
		*from = ntuple;
	}
	inline void gc_scan(Tuple *from)
	{
		for (size_t i = 0; i < from->m_count; i++)
		{
			gc_reallocate(&from->m_data[i]);
		}
	}

	inline Tuple *join_tuple(const Tuple *l, const Tuple *r)
	{
		Tuple *ret = new_tuple(l->m_count + r->m_count);
		std::copy(l->begin(), l->end(), ret->m_data);
		std::copy(r->begin(), r->end(), ret->m_data + l->m_count);
		return ret;
	}

	inline Value value(const std::vector<Value> &tuple) { return make_value_ptr(TUPLE, new_tuple(tuple)); }
	inline Value value(const Tuple *tuple) { return make_value_ptr(TUPLE, tuple); }
}