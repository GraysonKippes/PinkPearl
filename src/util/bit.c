#include "bit.h"

const uint64_t flags_bitwidth = UINT64_MAX;

bool is_bit_on(uint64_t x, uint64_t bit_pos) {
	return !!(x & (1LL << bit_pos));
}

bool is_bit_off(uint64_t x, uint64_t bit_pos) {
	return !(x & (1LL << bit_pos));
}

void set_bit_on(uint64_t x, uint64_t bit_pos) {
	x |= (1LL << bit_pos);
}

void set_bit_off(uint64_t x, uint64_t bit_pos) {
	x &= ~(1LL << bit_pos);
}
