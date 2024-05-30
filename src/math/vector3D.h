#ifndef VECTOR3D_H
#define VECTOR3D_H

typedef struct vector3D_t {
	double x;	// Magnitude in the x direction.
	double y;	// Magnitude in the y direction.
	double z;	// Magnitude in the z direction.
} vector3D_t;

extern const vector3D_t zeroVector3D;

// Returns the vector sum a + b.
vector3D_t vector3D_add(const vector3D_t a, const vector3D_t b);

// Returns the vector sum a - b.
vector3D_t vector3D_subtract(const vector3D_t a, const vector3D_t b);

vector3D_t vector3D_scalar_multiply(const vector3D_t vector, const double scalar);
vector3D_t vector3D_scalar_divide(const vector3D_t vector, const double scalar);
double vector3D_length(const vector3D_t vector);
vector3D_t vector3D_normalize(const vector3D_t vector);

#endif	// VECTOR3D_H
