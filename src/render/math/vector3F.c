#include "vector3F.h"

const vector3F_t zero_vector3F = { 0.0F, 0.0F, 0.0F };

vector3F_t vector3F_add(const vector3F_t a, const vector3F_t b) {
	vector3F_t sum;
	sum.x = a.x + b.x;
	sum.y = a.y + b.y;
	sum.z = a.z + b.z;
	return sum;
}

