#ifndef RANDOM_H
#define RANDOM_H

#define LIST_OF_RANDOM_FUNCTIONS \
		X(short, randomS) \
		X(int, randomI) \
		X(long, randomL) \
		X(long long, randomLL) \
		X(unsigned short, randomUS) \
		X(unsigned int, randomU) \
		X(unsigned long, randomUL) \
		X(unsigned long long, randomULL)
		
#define X(typename, functionName) typename functionName(const typename minimum, const typename maximum);
LIST_OF_RANDOM_FUNCTIONS
#undef X

#define random(minimum, maximum) _Generic((minimum), \
			short: randomS, \
			int: randomI, \
			long: randomL, \
			long long: randomLL, \
			unsigned short: randomUS, \
			unsigned int: randomU, \
			unsigned long: randomUL, \
			unsigned long long: randomULL \
		)(minimum, (typeof(minimum))maximum)

void initRandom(void);

#endif	// RANDOM_H