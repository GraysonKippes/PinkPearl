#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

extern const double pi;

typedef struct vector3D_rectangular_t {
	double x;	// Magnitude in the x direction.
	double y;	// Magnitude in the y direction.
	double z;	// Magnitude in the z direction.
} vector3D_rectangular_t;

typedef struct vector3D_polar_t {
	double r;	// Radial line, or the magnitude of this vector.
	double theta;	// Polar angle, or the angle between z-axis and the radial line.
	double phi;	// Azimuthal angle, or the angle between the orthongal projection of the radial line onto the x-y plane and either the x-axis or the y-axis.
} vector3D_polar_t;

vector3D_polar_t vector3D_rectangular_to_polar(vector3D_rectangular_t vector);

vector3D_rectangular_t vector3D_polar_to_rectangular(vector3D_polar_t vector);

vector3D_rectangular_t vector3D_rectangular_add(vector3D_rectangular_t a, vector3D_rectangular_t b);

typedef struct entity_transform_t {

	vector3D_rectangular_t position;

	vector3D_polar_t velocity;

} entity_transform_t;

#endif	// ENTITY_TRANSFORM_H
