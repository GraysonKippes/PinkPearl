#include "bit.h"

const uint64_t flags_bitwidth = UINT64_MAX;

bool is_bit_on(uint64_t x, uint64_t bit_pos) {
	return bit_pos < flags_bitwidth && !!(x & (1LL << bit_pos));
}

bool is_bit_off(uint64_t x, uint64_t bit_pos) {
	return bit_pos < flags_bitwidth && !(x & (1LL << bit_pos));
}

uint64_t set_bit_on(uint64_t x, uint64_t bit_pos) {
	return bit_pos < flags_bitwidth 
		? x | (1LL << bit_pos) 
		: x;
}

uint64_t set_bit_off(uint64_t x, uint64_t bit_pos) {
	return bit_pos < flags_bitwidth 
		? x & ~(1LL << bit_pos) 
		: x;
}
