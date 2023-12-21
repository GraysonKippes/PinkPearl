#include "entity_transform.h"

#include <math.h>

const double pi = 3.14159265358979323846;

vector3D_spherical_t vector3D_cubic_to_spherical(vector3D_cubic_t vector) {

	vector3D_spherical_t vector_spherical;

	vector_spherical.r = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);

	if (vector.x > 0.0) {
		vector_spherical.phi = atan(vector.y / vector.x);
	}
	else if (vector.x < 0.0) {
		if (vector.y > 0.0) {
			vector_spherical.phi = atan(vector.y / vector.x) + pi;
		}
		else {
			vector_spherical.phi = atan(vector.y / vector.x) - pi;
		}
	}
	else {
		if (vector.y > 0.0) {
			vector_spherical.phi = pi / 2.0;
		}
		else {
			vector_spherical.phi = -pi / 2.0;
		}
	}

	if (vector.z > 0.0) {
		vector_spherical.theta = atan(sqrt(vector.x * vector.x + vector.y * vector.y) / vector.z);
	}
	else if (vector.z < 0.0) {
		vector_spherical.theta = atan(sqrt(vector.x * vector.x + vector.y * vector.y) / vector.z) + pi;
	}
	else {
		vector_spherical.theta = pi / 2.0;
	}

	return vector_spherical;
}

vector3D_cubic_t vector3D_spherical_to_cubic(vector3D_spherical_t vector) {
	return (vector3D_cubic_t){
		.x = vector.r * sin(vector.theta) * cos(vector.phi),
		.y = vector.r * sin(vector.theta) * sin(vector.phi),
		.z = vector.r * cos(vector.theta)
	};
}

vector3D_cubic_t vector3D_cubic_add(vector3D_cubic_t a, vector3D_cubic_t b) {
	return (vector3D_cubic_t){
		.x = a.x + b.x,
		.y = a.y + b.y,
		.z = a.z + b.z
	};
}
