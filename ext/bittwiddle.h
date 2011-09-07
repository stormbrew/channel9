
#pragma once

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

