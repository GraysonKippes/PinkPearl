#include "vector4F.h"

#include "lerp.h"

const vector4F_t vector4F_zero = { 0.0F, 0.0F, 0.0F, 0.0F };

vector4F_t vector4F_add(const vector4F_t a, const vector4F_t b) {
	return (vector4F_t){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w
	};
}

vector4F_t vector4F_lerp(const vector4F_t start, const vector4F_t end, const float delta) {
	return (vector4F_t){
		.x = lerpF(start.x, end.x, delta),
		.y = lerpF(start.y, end.y, delta),
		.z = lerpF(start.z, end.z, delta),
		.w = lerpF(start.w, end.w, delta)
	};
}