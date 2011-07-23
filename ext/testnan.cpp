#include <stdio.h>
#include <math.h>

#include <string>
#include <sstream>

template <class T> T from_str(const std::string & str){
	std::istringstream sin(str);
	T ret;
	sin >> ret;
	return ret;
}

void print(double a){
	printf("%.20f, %llX\n", a, *((unsigned long long *) &a));
}

int main(int argc, char **argv){
	double i = from_str<double>(argv[1]);

	print(sqrt(-1));
	print(1.0/i);
	print(-1.0/i);
	print(0.0/i);
	print(-0.0/i);
	print((1.0/i)/(-1.0/i));

	print((1.0/i) + 3.0);
	print((-1.0/i) + 3.0);

	print(3.0/(1.0/i));

	return 0;
}

