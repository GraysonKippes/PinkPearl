#include "vector3F.h"

#include "lerp.h"

const vector3F_t zero_vector3F = { 0.0F, 0.0F, 0.0F };

vector3F_t vector3F_add(const vector3F_t a, const vector3F_t b) {
	return (vector3F_t){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

vector3F_t vector3F_lerp(const vector3F_t start, const vector3F_t end, const double delta) {
	return (vector3F_t){
		.x = lerpF(start.x, end.x, delta),
		.y = lerpF(start.y, end.y, delta),
		.z = lerpF(start.z, end.z, delta)
	};
}