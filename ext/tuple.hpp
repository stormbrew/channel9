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
		Tuple *ret = value_pool.alloc<Tuple>(sizeof(Value)*len, TUPLE);
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

	inline void gc_scan(Tuple *from)
	{
		for (size_t i = 0; i < from->m_count; i++)
		{
			gc_scan(from->m_data[i]);
		}
	}

	inline Tuple *join_tuple(const Tuple *l, const Tuple *r)
	{
		Tuple *ret = new_tuple(l->m_count + r->m_count);
		std::copy(l->begin(), l->end(), ret->m_data);
		std::copy(r->begin(), r->end(), ret->m_data + l->m_count);
		return ret;
	}

	inline Tuple *join_tuple(const Value &l, const Tuple *r)
	{
		Tuple *ret = new_tuple(1 + r->m_count);
		ret->m_data[0] = l;
		std::copy(r->begin(), r->end(), ret->m_data + 1);
		return ret;
	}
	inline Tuple *join_tuple(const Tuple *l, const Value &r)
	{
		Tuple *ret = new_tuple(l->m_count + 1);
		ret->m_data[l->m_count] = r;
		std::copy(l->begin(), l->end(), ret->m_data);
		return ret;
	}

	inline Tuple *sub_tuple(const Tuple *t, size_t first, size_t count)
	{
		Tuple *ret = new_tuple(count);
		std::copy(t->begin() + first, t->begin() + first + count, ret->m_data);
		return ret;
	}

	inline Tuple *replace_tuple(const Tuple *t, size_t pos, const Value &val)
	{
		Tuple *ret = new_tuple(std::max(t->m_count, pos+1));
		std::copy(t->begin(), t->begin() + std::min(pos, t->m_count), ret->m_data);
		if (pos < t->m_count)
			std::copy(t->begin() + pos + 1, t->end(), ret->m_data + pos + 1);
		std::fill(ret->begin() + t->m_count, ret->end(), Nil);
		ret->m_data[pos] = val;
		return ret;
	}
	inline Tuple *remove_tuple(const Tuple *t, size_t pos)
	{
		Tuple *ret = new_tuple(t->m_count - 1);
		std::copy(t->begin(), t->begin() + pos, ret->m_data);
		std::copy(t->begin() + pos + 1, t->end(), ret->m_data + pos);
		return ret;
	}

	inline Tuple *split_string(const String *s, const String *by)
	{
		std::vector<Value> strings;
		String::const_iterator first = s->begin(), next;
		while (first < s->end())
		{
			next = std::search(first, s->end(), by->begin(), by->end());
			strings.push_back(value(new_string(first, next)));
			first = next + by->m_count;
		}
		return new_tuple(strings.begin(), strings.end());
	}

	inline Value value(const std::vector<Value> &tuple) { return make_value_ptr(TUPLE, new_tuple(tuple)); }
	inline Value value(const Tuple *tuple) { return make_value_ptr(TUPLE, tuple); }
}

