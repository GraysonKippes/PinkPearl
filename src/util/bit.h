#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>

#define IS_BIT_ON(x, pos) ((pos >> x) & 1)

#define TEST_MASK(x, mask) ((x & mask) >= 1)

extern const uint64_t flags_bitwidth;

// Returns true if the bit at the specified position in `x` is one;
// 	returns false otherwise.
bool is_bit_on(uint64_t x, uint64_t bit_pos);

// Returns true if the bit at the specified position in `x` is zero;
// 	returns false otherwise.
bool is_bit_off(uint64_t x, uint64_t bit_pos);

// Returns `x` with the bit at the specified position in `x` to one.
uint64_t set_bit_on(uint64_t x, uint64_t bit_pos);

// Returns `x` with the bit at the specified position in `x` to zero.
uint64_t set_bit_off(uint64_t x, uint64_t bit_pos);

#endif	// UTIL_H
