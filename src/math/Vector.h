#ifndef MATH_VECTOR_H
#define MATH_VECTOR_H

#ifndef __STDC_NO_COMPLEX__
#include <complex.h>
#endif
#include <math.h>

// Vector operation macros

#define addVec(a, b) _Generic((a), \
			Vector3D: addVec3D, \
			Vector4F: addVec4F \
		)(a, b)

#define subVec(a, b) _Generic((a), \
			Vector3D: subVec3D, \
			Vector4F: subVec4F \
		)(a, b)

#define mulVec(a, b) _Generic((a), \
			Vector3D: mulVec3D, \
			Vector4F: mulVec4F \
		)(a, b)

#define divVec(a, b) _Generic((a), \
			Vector3D: divVec3D, \
			Vector4F: divVec4F \
		)(a, b)

#define magnitude(a) _Generic((a), \
			float: fabsf, \
			default: fabs, \
			long double: fabsl, \
			Vector3D: magnitudeVec3D, \
			Vector4F: magnitudeVec4F \
		)(a)

#define normVec(v) _Generic((v), \
			Vector3D: normVec3D, \
			Vector4F: normVec4F \
		)(v)

// Type definitions

typedef struct Vector3D {
	double x;
	double y;
	double z;
} Vector3D;

typedef struct Vector4F {
	float x;
	float y;
	float z;
	float w;
} Vector4F;

// Common vectors (zero vectors, unit vectors)

extern const Vector3D zeroVec3D;
extern const Vector4F zeroVec4F;

extern const Vector3D unitVec3DPosX; // +i
extern const Vector3D unitVec3DNegX; // -i
extern const Vector3D unitVec3DPosY; // +j
extern const Vector3D unitVec3DNegY; // -j
extern const Vector3D unitVec3DPosZ; // +k
extern const Vector3D unitVec3DNegZ; // -k

// Vector3D operations

// Returns true if the all elements of v are zero or close enough to zero within a set tolerance.
bool isZeroVec3D(const Vector3D v);

// Constructs and returns a three component vector of double-precision floats.
Vector3D makeVec3D(const double x, const double y, const double z);

// Returns the vector sum a + b.
Vector3D addVec3D(const Vector3D a, const Vector3D b);

// Returns the vector difference a - b.
Vector3D subVec3D(const Vector3D a, const Vector3D b);

// Returns the scaled vector cv.
Vector3D mulVec3D(const Vector3D v, const double c);

// Returns the scaled vector v/c.
Vector3D divVec3D(const Vector3D v, const double c);

// Returns the magnitude of the vector v.
double magnitudeVec3D(const Vector3D v);

// Returns the normalization of vector v.
Vector3D normVec3D(const Vector3D v);

// Vector4F operations

// Returns true if the all elements of v are zero or close enough to zero within a set tolerance.
bool isZeroVec4F(const Vector4F v);

// Constructs and returns a four component vector of single-precision floats.
Vector4F makeVec4F(const float x, const float y, const float z, const float w);

// Returns the vector sum a + b.
Vector4F addVec4F(const Vector4F a, const Vector4F b);

// Returns the vector difference a - b.
Vector4F subVec4F(const Vector4F a, const Vector4F b);

// Returns the scaled vector cv.
Vector4F mulVec4F(const Vector4F v, const float c);

// Returns the scaled vector v/c.
Vector4F divVec4F(const Vector4F v, const float c);

// Returns the linear interpolation between two vectors.
Vector4F lerpVec4F(const Vector4F start, const Vector4F end, const float delta);

// Returns the magnitude of the vector v.
float magnitudeVec4F(const Vector4F v);

// Returns the normalization of vector v.
Vector4F normVec4F(const Vector4F v);

// Vector conversions

Vector3D vec4FtoVec3D(const Vector4F v);

Vector4F vec3DtoVec4F(const Vector3D v);

#endif	// MATH_VECTOR_H