#include "vector3D.h"

#include <math.h>

const Vector3D zeroVector3D = (Vector3D){ 0.0, 0.0, 0.0 };

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
	const double length = vector3D_length(vector);
	if (length == 0.0) {
		return (Vector3D){ 0.0 };
	}
	return vector3D_scalar_divide(vector, length);
}