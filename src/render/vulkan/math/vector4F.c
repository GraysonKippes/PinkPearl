#include "vector4F.h"

#include "lerp.h"

const Vector4F vector4F_zero = { 0.0F, 0.0F, 0.0F, 1.0F };

Vector4F vector4F_add(const Vector4F a, const Vector4F b) {
	return (Vector4F){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w
	};
}

Vector4F vector4F_lerp(const Vector4F start, const Vector4F end, const float delta) {
	return (Vector4F){
		.x = lerpF(start.x, end.x, delta),
		.y = lerpF(start.y, end.y, delta),
		.z = lerpF(start.z, end.z, delta),
		.w = lerpF(start.w, end.w, delta)
	};
}

const Transform transformZero = {
	.translation = vector4F_zero,
	.scaling = vector4F_zero,
	.rotation = vector4F_zero
};