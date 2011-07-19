#pragma once

#include <set>

namespace Channel9
{
	class MemoryPool;

	// Base class for GC root objects. Should inherit privately.
	class GCRoot
	{
	private:
		MemoryPool &m_pool;

	protected:
		GCRoot(MemoryPool &pool);

		MemoryPool &pool() const { return m_pool; }

	public:
		virtual void scan() = 0;

		virtual ~GCRoot();
	};

	class MemoryPool
	{
	private:
		unsigned char *m_cur_pool;
		unsigned char *m_pos;
		size_t m_remains;
		size_t m_size;

		std::set<GCRoot*> m_roots;

		void collect()
		{
			std::set<GCRoot*>::iterator it;
			for (it = m_roots.begin(); it != m_roots.end(); it++)
			{
				(*it)->scan();
			}
		}

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
				unsigned char *old_pool = m_cur_pool;

				m_size = m_size + m_size / 2;
				
				m_cur_pool = (unsigned char *)malloc(m_size);
				m_pos = m_cur_pool + size;
				m_remains = m_size - size;

				collect();

				free(old_pool);

				unsigned char *ret = m_pos;
				m_pos += size;
				m_remains -= size;
				return ret;
			}
		}

	public:
		MemoryPool(size_t initial_size) 
		 : m_cur_pool(NULL), m_pos(NULL), m_remains(0), m_size(initial_size) 
		{}

		template <typename tObj>
		tObj *alloc(size_t extra = 0)
		{
			return reinterpret_cast<tObj*>(next(sizeof(tObj) + extra));
		}

		void unregister_root(GCRoot *root);
		void register_root(GCRoot *root);
	};

	extern MemoryPool value_pool;
}