#ifndef UTIL_H
#define UTIL_H

#define IS_BIT_ON(x, pos) ((pos >> x) & 1)

#define TEST_MASK(x, mask) ((x & mask) >= 1)

#endif	// UTIL_H
