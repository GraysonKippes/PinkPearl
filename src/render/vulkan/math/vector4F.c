#include "vector4F.h"

#include "lerp.h"

const Vector4F zeroVector4F = { 0.0F, 0.0F, 0.0F, 1.0F };

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
		.x = interpolateLinear(start.x, end.x, delta),
		.y = interpolateLinear(start.y, end.y, delta),
		.z = interpolateLinear(start.z, end.z, delta),
		.w = interpolateLinear(start.w, end.w, delta)
	};
}

const Transform transformZero = {
	.translation = zeroVector4F,
	.scaling = zeroVector4F,
	.rotation = zeroVector4F
};