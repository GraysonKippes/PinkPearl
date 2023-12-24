#ifndef VECTOR3D_H
#define VECTOR3D_H

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

// Returns the vector sum a + b.
vector3D_cubic_t vector3D_cubic_add(const vector3D_cubic_t a, const vector3D_cubic_t b);

// Returns the vector sum a - b.
vector3D_cubic_t vector3D_cubic_subtract(const vector3D_cubic_t a, const vector3D_cubic_t b);

#endif	// VECTOR3D_H
