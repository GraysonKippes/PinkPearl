#include "Vector.h"

#define LERP(start, end, delta) ((start) + (delta) * ((end) - (start)))

const Vector3D zeroVec3D = { 0.0, 0.0, 0.0 };

const Vector4F zeroVec4F = { 0.0F, 0.0F, 0.0F, 1.0F };

bool isZeroVec3D(const Vector3D v) {
	static const double tolerance = 0.0001;
	return fabs(v.x) < tolerance && fabs(v.y) < tolerance && fabs(v.z) < tolerance;
}

Vector3D addVec3D(const Vector3D a, const Vector3D b) {
	return (Vector3D){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

Vector3D subVec3D(const Vector3D a, const Vector3D b) {
	return (Vector3D){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z
	};
}

Vector3D mulVec3D(const Vector3D v, const double c) {
	return (Vector3D){
		.x = v.x * c,
		.y = v.y * c,
		.z = v.z * c
	};
}

Vector3D divVec3D(const Vector3D v, const double c) {
	return (Vector3D){
		.x = v.x / c,
		.y = v.y / c,
		.z = v.z / c
	};
}

double magnitudeVec3D(const Vector3D v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vector3D normVec3D(const Vector3D v) {
	if (isZeroVec3D(v)) {
		return zeroVec3D;
	}
	return divVec3D(v, magnitudeVec3D(v));
}

bool isZeroVec4F(const Vector4F v) {
	static const float tolerance = 0.0001F;
	return fabs(v.x) < tolerance && fabs(v.y) < tolerance && fabs(v.z) < tolerance && fabs(v.w) < tolerance;
}

Vector4F addVec4F(const Vector4F a, const Vector4F b) {
	return (Vector4F){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z,
		.w = a.w + b.w
	};
}

Vector4F subVec4F(const Vector4F a, const Vector4F b) {
	return (Vector4F){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z,
		.w = a.w - b.w
	};
}

Vector4F mulVec4F(const Vector4F v, const float c) {
	return (Vector4F){
		.x = v.x * c,
		.y = v.y * c,
		.z = v.z * c,
		.w = v.w * c
	};
}

Vector4F divVec4F(const Vector4F v, const float c) {
	return (Vector4F){
		.x = v.x / c,
		.y = v.y / c,
		.z = v.z / c,
		.w = v.w / c
	};
}

Vector4F lerpVec4F(const Vector4F start, const Vector4F end, const float delta) {
	return (Vector4F){
		.x = LERP(start.x, end.x, delta),
		.y = LERP(start.y, end.y, delta),
		.z = LERP(start.z, end.z, delta),
		.w = LERP(start.w, end.w, delta)
	};
}

float magnitudeVec4F(const Vector4F v) {
	return sqrt(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
}

Vector4F normVec4F(const Vector4F v) {
	if (isZeroVec4F(v)) {
		return zeroVec4F;
	}
	return divVec4F(v, magnitudeVec4F(v));
}

Vector3D vec4FtoVec3D(const Vector4F v) {
	return (Vector3D){
		.x = (double)v.x,
		.y = (double)v.y,
		.z = (double)v.z
	};
}

Vector4F vec3DtoVec4F(const Vector3D v) {
	return (Vector4F){
		.x = (float)v.x,
		.y = (float)v.y,
		.z = (float)v.z,
		.w = 1.0F
	};
}