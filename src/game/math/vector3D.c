#include "vector3D.h"

#include <math.h>

vector3D_t vector3D_add(const vector3D_t a, const vector3D_t b) {
	return (vector3D_t){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}

vector3D_t vector3D_subtract(const vector3D_t a, const vector3D_t b) {
	return (vector3D_t){
		.x = a.x - b.x,
		.y = a.y - b.y,
		.z = a.z - b.z
	};
}

vector3D_t vector3D_scalar_multiply(const vector3D_t vector, const double scalar) {
	return (vector3D_t){
		.x = vector.x * scalar,
		.y = vector.y * scalar,
		.z = vector.z * scalar
	};
}

vector3D_t vector3D_scalar_divide(const vector3D_t vector, const double scalar) {
	return (vector3D_t){
		.x = vector.x / scalar,
		.y = vector.y / scalar,
		.z = vector.z / scalar
	};
}

double vector3D_length(const vector3D_t vector) {
	return sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
}

vector3D_t vector3D_normalize(const vector3D_t vector) {
	const double length = vector3D_length(vector);
	if (length == 0.0) {
		return (vector3D_t){ 0.0 };
	}
	return vector3D_scalar_divide(vector, length);
}