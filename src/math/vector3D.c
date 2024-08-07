#include "vector3D.h"

#include <math.h>

const Vector3D zeroVector3D = { 0.0, 0.0, 0.0 };

bool vector3DIsZero(const Vector3D vector) {
	static const double tolerance = 0.0001;
	return fabs(vector.x) < tolerance && fabs(vector.y) < tolerance && fabs(vector.z) < tolerance;
}

Vector3D vector3D_add(const Vector3D a, const Vector3D b) {
	return (Vector3D){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

Vector3D vector3D_subtract(const Vector3D a, const Vector3D b) {
	return (Vector3D){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z
	};
}

Vector3D vector3D_scalar_multiply(const Vector3D vector, const double scalar) {
	return (Vector3D){
		.x = vector.x * scalar,
		.y = vector.y * scalar,
		.z = vector.z * scalar
	};
}

Vector3D vector3D_scalar_divide(const Vector3D vector, const double scalar) {
	return (Vector3D){
		.x = vector.x / scalar,
		.y = vector.y / scalar,
		.z = vector.z / scalar
	};
}

double vector3D_length(const Vector3D vector) {
	return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

Vector3D vector3D_normalize(const Vector3D vector) {
	if (vector3DIsZero(vector)) {
		return zeroVector3D;
	}
	const double length = vector3D_length(vector);
	return vector3D_scalar_divide(vector, length);
}