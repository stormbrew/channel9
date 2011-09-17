#pragma once

namespace Channel9
{
	//round a number up to the nearest power of 2
	template<typename uint>
	inline uint ceil_power2(uint v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		if(sizeof(uint) >= 2) v |= v >> 8;
		if(sizeof(uint) >= 4) v |= v >> 16;
		if(sizeof(uint) >= 8) v |= v >> 32;
		v++;
		return v;
	}

	inline uint64_t ceil_power2(uint64_t v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v |= v >> 32;
		v++;
		return v;
	}
	inline uint32_t ceil_power2(uint32_t v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}
	inline uint16_t ceil_power2(uint16_t v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v++;
		return v;
	}
	inline uint8_t ceil_power2(uint8_t v) {
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v++;
		return v;
	}



	//returns true if exactly 1 bit is set
	template<typename uint>
	inline bool is_power2(uint n){
		return (n && !(n & (n - 1)));
	}

	template<typename uint>
	inline uint clear_bottom_bits(uint n, int num_bits){
		return (n & ~((1 << num_bits)-1));
	}


	//count the trailing zeroes, essentially what power of 2 is it
	template<typename uint>
	inline uint count_bottom_zeros1(uint n){
		if(n == 0)
			return 0;//sizeof(uint) * 8;

		uint count = 0;
		while((n & 1) == 0){
			++count;
			n >>= 1;
		}
		return count;
	}

	//count the trailing zeroes, essentially what power of 2 is it
	template<typename uint>
	inline uint count_bottom_zeros2(uint n){
		if(n == 0)
			return 0;//sizeof(uint) * 8;

		uint count = 0;
		uint bits = sizeof(uint) * 4;
		do {
			if((n & ((1 << bits) - 1)) == 0){
				count += bits;
				n >>= bits;
			}
			bits >>= 1;
		}while(bits);

		return count;
	}

	//count the trailing zeroes, essentially what power of 2 is it
	template<typename uint>
	inline uint count_bottom_zeros3(uint n){
		if(n == 0)
			return 0;//sizeof(uint) * 8;

		uint count = 0;
		uint bits = sizeof(uint) * 4;
		uint mask = (1 << bits) - 1;
		do {
			if((n & mask) == 0){
				count += bits;
				n >>= bits;
			}
			bits >>= 1;
			mask >>= bits;
		}while(bits && (n & 1) == 0);

		return count;
	}

	//count the trailing zeroes, essentially what power of 2 is it
	template<typename uint>
	inline uint count_bottom_zeros4(uint n){
		if(n == 0)
			return 0; //sizeof(uint) * 8;

		uint count = 0;

		if(sizeof(uint) >= 8 && (n & 0xFFFFFFFFULL) == 0){ count += 32; n >>= 32; }
		if(sizeof(uint) >= 4 && (n & 0x0000FFFFULL) == 0){ count += 16; n >>= 16; }
		if(sizeof(uint) >= 2 && (n & 0x000000FFULL) == 0){ count +=  8; n >>=  8; }
		if(sizeof(uint) >= 1 && (n & 0x0000000FULL) == 0){ count +=  4; n >>=  4; }
		if(sizeof(uint) >= 1 && (n & 0x00000003ULL) == 0){ count +=  2; n >>=  2; }
		if(sizeof(uint) >= 1 && (n & 0x00000001ULL) == 0){ count +=  1; n >>=  1; }

		return count;
	}
	inline uint8_t count_bottom_zeros4(uint8_t n){
		if(n == 0)
			return 0;
		uint8_t count = 0;
		if((n & 0x0000000FULL) == 0){ count +=  4; n >>=  4; }
		if((n & 0x00000003ULL) == 0){ count +=  2; n >>=  2; }
		if((n & 0x00000001ULL) == 0){ count +=  1; n >>=  1; }
		return count;
	}
	inline uint16_t count_bottom_zeros4(uint16_t n){
		if(n == 0)
			return 0;
		uint16_t count = 0;
		if((n & 0x000000FFULL) == 0){ count +=  8; n >>=  8; }
		if((n & 0x0000000FULL) == 0){ count +=  4; n >>=  4; }
		if((n & 0x00000003ULL) == 0){ count +=  2; n >>=  2; }
		if((n & 0x00000001ULL) == 0){ count +=  1; n >>=  1; }
		return count;
	}
	inline uint32_t count_bottom_zeros4(uint32_t n){
		if(n == 0)
			return 0;
		uint32_t count = 0;
		if((n & 0x0000FFFFULL) == 0){ count += 16; n >>= 16; }
		if((n & 0x000000FFULL) == 0){ count +=  8; n >>=  8; }
		if((n & 0x0000000FULL) == 0){ count +=  4; n >>=  4; }
		if((n & 0x00000003ULL) == 0){ count +=  2; n >>=  2; }
		if((n & 0x00000001ULL) == 0){ count +=  1; n >>=  1; }
		return count;
	}
	inline uint64_t count_bottom_zeros4(uint64_t n){
		if(n == 0)
			return 0;
		uint64_t count = 0;
		if((n & 0xFFFFFFFFULL) == 0){ count += 32; n >>= 32; }
		if((n & 0x0000FFFFULL) == 0){ count += 16; n >>= 16; }
		if((n & 0x000000FFULL) == 0){ count +=  8; n >>=  8; }
		if((n & 0x0000000FULL) == 0){ count +=  4; n >>=  4; }
		if((n & 0x00000003ULL) == 0){ count +=  2; n >>=  2; }
		if((n & 0x00000001ULL) == 0){ count +=  1; n >>=  1; }
		return count;
	}


	//count how many bits are set
	template<typename uint>
	inline uint count_bits_set(uint n){
		uint count = 0;
		while(n){
			++count;
			n &= (n - 1);
		}
		return count;
	}

	//give a good distribution over the hash space to have fewer collisions, both copied from MurmurHash
	inline uint32_t mix_bits(uint32_t h){
		h ^= h >> 16;
		h *= 0x85ebca6b;
		h ^= h >> 13;
		h *= 0xc2b2ae35;
		h ^= h >> 16;
		return h;
	}
	inline uint64_t mix_bits(uint64_t h){
		h ^= h >> 33;
		h *= 0xff51afd7ed558ccd;
		h ^= h >> 33;
		h *= 0xc4ceb9fe1a85ec53;
		h ^= h >> 33;
		return h;
	}
}
