#pragma once

#include <stdlib.h>

namespace Channel9
{
	template <size_t area_size>
	class MemoryPool
	{
	private:
		unsigned char *m_cur_pool;
		unsigned char *m_pos;
		size_t m_remains;

		unsigned char *next(size_t size)
		{
			size += 8 - size % 8;
			if (m_remains >= size)
			{
				unsigned char *ret = m_pos;
				m_pos += size;
				m_remains -= size;
				return ret;
			} else {
				// TODO: Proper garbage collection.
				m_cur_pool = (unsigned char *)malloc(area_size);
				m_pos = m_cur_pool + size;
				m_remains = area_size - size;
				return m_cur_pool;
			}
		}

	public:
		MemoryPool() : m_cur_pool(NULL), m_pos(NULL), m_remains(0) {}

		template <typename tObj>
		tObj *alloc(size_t extra = 0)
		{
			return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra));
		}
	};

	extern MemoryPool<8*1024*1024> value_pool;
}

