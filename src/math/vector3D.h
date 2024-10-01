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
Vector3D vector3DAdd(const Vector3D a, const Vector3D b);

// Returns the vector sum a - b.
Vector3D vector3DSubtract(const Vector3D a, const Vector3D b);

Vector3D vector3DScalarMultiply(const Vector3D vector, const double scalar);

Vector3D vector3DScalarDivide(const Vector3D vector, const double scalar);

double vector3DLength(const Vector3D vector);

Vector3D vector3DNormalize(const Vector3D vector);

#endif	// VECTOR3D_H
