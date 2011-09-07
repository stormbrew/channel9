
#include <iostream>
//#include <cassert>
#include "time.h"
#include "../bitfiddle.h"

using namespace std;

int main(){
/*
	assert(count_bits_set(1) == 1);
	assert(count_bits_set(2) == 1);
	assert(count_bits_set(3) == 2);
	assert(count_bits_set(13) == 3);
	assert(count_bits_set(435) == 6);
	assert(count_bits_set(512) == 1);


	assert(is_power2(1));
	assert(is_power2(2));
	assert(!is_power2(3));
	assert(is_power2(4));
	assert(!is_power2(5));
	assert(!is_power2(15));
	assert(is_power2(16));
	assert(!is_power2(435));

	for(int i = 0; i <= 256; i++)
		assert((count_bits_set(i) == 1) == is_power2(i));


	cout << "i\t1\t2\t3\t4\n";
	for(int i = 0; i <= 16; i++)
		cout << i << "\t" << count_bottom_zeros1(i) << "\t" << count_bottom_zeros2(i) << "\t" << count_bottom_zeros3(i) << "\t" << count_bottom_zeros4(i) << "\n";
*/
	unsigned int sum;

	unsigned int iters = 100000000;

#define shift(i) (i << (2+(i&7)))

	Time t1;

	sum = 0;
	for(unsigned int i = 0; i < iters; i++)
		sum += count_bottom_zeros1(shift(i));

	Time t2;
	cout << "Zeros1: " << (t2 - t1) << " seconds, sum: " << sum << "\n";

	sum = 0;
	for(unsigned int i = 0; i < iters; i++)
		sum += count_bottom_zeros2(shift(i));

	Time t3;
	cout << "Zeros2: " << (t3 - t2) << " seconds, sum: " << sum << "\n";

	sum = 0;
	for(unsigned int i = 0; i < iters; i++)
		sum += count_bottom_zeros3(shift(i));

	Time t4;
	cout << "Zeros3: " << (t4 - t3) << " seconds, sum: " << sum << "\n";

	sum = 0;
	for(unsigned int i = 0; i < iters; i++)
		sum += count_bottom_zeros4(shift(i));

	Time t5;
	cout << "Zeros4: " << (t5 - t4) << " seconds, sum: " << sum << "\n";
}

