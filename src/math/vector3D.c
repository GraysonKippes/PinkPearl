#include "vector3D.h"

#include <math.h>

const Vector3D zeroVector3D = { 0.0, 0.0, 0.0 };

bool vector3DIsZero(const Vector3D vector) {
	static const double tolerance = 0.0001;
	return fabs(vector.x) < tolerance && fabs(vector.y) < tolerance && fabs(vector.z) < tolerance;
}

Vector3D vector3DAdd(const Vector3D a, const Vector3D b) {
	return (Vector3D){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

Vector3D vector3DSubtract(const Vector3D a, const Vector3D b) {
	return (Vector3D){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z
	};
}

Vector3D vector3DScalarMultiply(const Vector3D vector, const double scalar) {
	return (Vector3D){
		.x = vector.x * scalar,
		.y = vector.y * scalar,
		.z = vector.z * scalar
	};
}

Vector3D vector3DScalarDivide(const Vector3D vector, const double scalar) {
	return (Vector3D){
		.x = vector.x / scalar,
		.y = vector.y / scalar,
		.z = vector.z / scalar
	};
}

double vector3DLength(const Vector3D vector) {
	return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

Vector3D vector3DNormalize(const Vector3D vector) {
	if (vector3DIsZero(vector)) {
		return zeroVector3D;
	}
	const double length = vector3DLength(vector);
	return vector3DScalarDivide(vector, length);
}