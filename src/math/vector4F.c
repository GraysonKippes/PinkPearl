#include "vector4F.h"

const vector4F_t vector4F_zero = { 0.0F, 0.0F, 0.0F, 0.0F };

vector4F_t vector4F_add(const vector4F_t a, const vector4F_t b) {
	return (vector4F_t){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w
	};
}