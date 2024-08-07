#ifndef VECTOR3D_H
#define VECTOR3D_H

typedef struct Vector3D {
	double x;	// Magnitude in the x direction.
	double y;	// Magnitude in the y direction.
	double z;	// Magnitude in the z direction.
} Vector3D;

extern const Vector3D zeroVector3D;

bool vector3DIsZero(const Vector3D vector);

// Returns the vector sum a + b.
Vector3D vector3D_add(const Vector3D a, const Vector3D b);

// Returns the vector sum a - b.
Vector3D vector3D_subtract(const Vector3D a, const Vector3D b);

Vector3D vector3D_scalar_multiply(const Vector3D vector, const double scalar);

Vector3D vector3D_scalar_divide(const Vector3D vector, const double scalar);

double vector3D_length(const Vector3D vector);

Vector3D vector3D_normalize(const Vector3D vector);

#endif	// VECTOR3D_H
