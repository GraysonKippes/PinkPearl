#include "entity_transform.h"

#include <math.h>

static const double pi = 3.14159265358979323846;

vector3D_polar_t vector3D_rectangular_to_polar(vector3D_rectangular_t vector) {

	vector3D_polar_t vector_polar;

	vector_polar.r = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);

	if (vector.z > 0.0) {
		vector_polar.theta = atan(sqrt(vector.x * vector.x + vector.y * vector.y) / vector.z);
	}
	else if (vector.z < 0.0) {
		vector_polar.theta = atan(sqrt(vector.x * vector.x + vector.y * vector.y) / vector.z) + pi;
	}
	else {
		vector_polar.theta = pi / 2.0;
	}

	if (vector.x > 0.0) {
		vector_polar.phi = atan(vector.y / vector.x);
	}
	else if (vector.x < 0.0) {
		if (vector.y > 0.0) {
			vector_polar.phi = atan(vector.y / vector.x) + pi;
		}
		else {
			vector_polar.phi = atan(vector.y / vector.x) - pi;
		}
	}
	else {
		if (vector.y > 0.0) {
			vector_polar.phi = pi / 2.0;
		}
		else {
			vector_polar.phi = -pi / 2.0;
		}
	}
}

vector3D_rectangular_t vector3D_polar_to_rectangular(vector3D_polar_t vector) {
	return (vector3D_rectangular_t){
		.x = vector.r * sin(vector.theta) * cos(vector.phi),
		.y = vector.r * sin(vector.theta) * sin(vector.phi),
		.z = vector.r * cos(vector.theta)
	};
}

