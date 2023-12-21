#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

#include <stdint.h>

extern const double pi;

typedef struct vector3D_cubic_t {
	double x;	// Magnitude in the x direction.
	double y;	// Magnitude in the y direction.
	double z;	// Magnitude in the z direction.
} vector3D_cubic_t;

typedef struct vector3D_spherical_t {
	double r;	// Radial line, or the magnitude of this vector.
	double theta;	// Polar angle, or the angle between z-axis and the radial line.
	double phi;	// Azimuthal angle, or the angle between the orthongal projection of the radial line onto the x-y plane and either the x-axis or the y-axis.
} vector3D_spherical_t;

vector3D_spherical_t vector3D_cubic_to_spherical(vector3D_cubic_t vector);

vector3D_cubic_t vector3D_spherical_to_cubic(vector3D_spherical_t vector);

vector3D_cubic_t vector3D_cubic_add(vector3D_cubic_t a, vector3D_cubic_t b);

typedef struct entity_transform_t {

	vector3D_cubic_t position;

	vector3D_spherical_t velocity;

	uint64_t last_stationary_time;

} entity_transform_t;

#endif	// ENTITY_TRANSFORM_H
