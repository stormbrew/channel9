
#pragma once

#include "channel9.hpp"
#include "bittwiddle.hpp"

namespace Channel9
{
	class ForwardTable {
		struct Entry {
			uintptr_t from, to;
			Entry(uintptr_t f = 0, uintptr_t t = 0) : from(f), to(t) { }
		};

		uintptr_t size; //how many slots
		uintptr_t num;  //how many are used
		uintptr_t mask;
		Entry * table;

	public:
		ForwardTable() : size(0), mask(0), table(NULL) { }
		ForwardTable(uintptr_t s){ init(s); }
		~ForwardTable(){
			clear();
		}

		void init(uintptr_t s){
			clear();
			size = ceil_power2(s)*4;
			mask = size-1;
			table = new Entry[size];
		}

		void clear(){
			if(table)
				delete[] table;
			table = NULL;
		}

		void clean(){
			for(Entry *i = table, *end = table + size; i != end; ++i)
				*i = Entry();
		}

		template<typename tObj>
		void set(tObj * fromptr, tObj * toptr){ return set((uintptr_t) fromptr, (uintptr_t) toptr); }
		void set(uintptr_t from, uintptr_t to){
			uintptr_t i = mix_bits(from) & mask;
			while(table[i].from != 0)
				i = (i+1) & mask;
			table[i] = Entry(from, to);
			num++;
		}

		template<typename tObj>
		tObj *    get(tObj * fromptr){ return (tObj *) get((uintptr_t) fromptr); }
		uintptr_t get(uintptr_t from){
			for(uintptr_t i = mix_bits(from) & mask; table[i].from; i = (i+1) & mask)
				if(table[i].from == from)
					return table[i].to;
			return 0;
		}
	};
}

