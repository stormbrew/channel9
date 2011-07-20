#pragma once

#include "memory_pool.hpp"

#include <string>
#include <string.h>
#include <algorithm>
#include <assert.h>

namespace Channel9
{
	struct String
	{
		size_t m_count;
		char m_data[0];

		size_t count() const { return m_count; }
		size_t size() const { return m_count; }
		size_t length() const { return m_count; }

		typedef char *iterator;
		typedef const char *const_iterator;

		iterator begin() { return m_data; }
		iterator end() { return m_data + m_count; }
		const_iterator begin() const { return m_data; }
		const_iterator end() const { return m_data + m_count; }

		char operator[](size_t idx) const { return m_data[idx]; }

		String *substr(size_t off, size_t len) const;

		const char *c_str() const
		{
			const_cast<char*>(m_data)[m_count] = 0;
			return m_data;
		}
		const std::string str() const
		{
			return std::string(m_data, m_data + m_count);
		}

		bool equal(const String &r) const
		{
			return this == &r || (m_count == r.m_count && std::equal(m_data, m_data + m_count, r.m_data));
		}
		bool equal(const std::string &str) const
		{
			return m_count == str.length() && std::equal(str.begin(), str.end(), m_data);
		}
		bool equal(const char *str) const
		{
			size_t len = strlen(str);
			return len == m_count && std::equal(str, str + len, m_data);
		}
	};

	inline bool operator==(const String &l, const String &r)
	{
		return l.equal(r);
	}
	inline bool operator!=(const String &l, const String &r)
	{
		return !l.equal(r);
	}
	template <typename tOther>
	inline bool operator==(const String &l, const tOther &r)
	{
		return l.equal(r);
	}
	template <typename tOther>
	inline bool operator!=(const String &l, const tOther &r)
	{
		return !l.equal(r);
	}
	template <typename tOther>
	inline bool operator==(const tOther &r, const String &l)
	{
		return l.equal(r);
	}
	template <typename tOther>
	inline bool operator!=(const tOther &r, const String &l)
	{
		return !l.equal(r);
	}

	inline String *new_string(size_t len)
	{
		String *ret = value_pool.alloc<String>(len + 1);
		ret->m_count = len;

		return ret;
	}
	template <typename tIter>
	inline String *new_string(size_t len, tIter str)
	{
		String *ret = new_string(len);
		std::copy(str, str + len, ret->m_data);
		return ret;
	}
	template <typename tIter>
	inline String *new_string(tIter beg, tIter end)
	{
		String *ret = new_string(end - beg);
		std::copy(beg, end, ret->m_data);
		return ret;
	}
	inline String *new_string(const char *zstr)
	{
		return new_string(strlen(zstr), zstr);
	}
	inline String *new_string(const std::string &str)
	{
		return new_string(str.begin(), str.end());
	}

	inline void gc_reallocate(String **from)
	{
		String *nstr = value_pool.alloc<String>((*from)->m_count);
		memcpy(nstr, *from, sizeof(String) + (*from)->m_count);
		*from = nstr;
	}
	inline void gc_scan(String *from)
	{
		// nothing to scan. Here for completeness' sake.
	}

	inline String *join_string(const String *l, const String *r)
	{
		String *ret = new_string(l->m_count + r->m_count);
		std::copy(l->begin(), l->end(), ret->m_data);
		std::copy(r->begin(), r->end(), ret->m_data + l->m_count);
		return ret;
	}

	inline String *String::substr(size_t off, size_t len) const
	{
		assert(off + len < m_count);
		String *ret = new_string(len);
		std::copy(m_data + off, m_data + off + len, ret->m_data);
		return ret;
	}

	template <typename tOut>
	inline tOut &operator<<(tOut &out, const String &str)
	{
		out << str.c_str();
		return out;
	}
}

