#ifndef VECTOR3F_H
#define VECTOR3F_H

// Single-precision 3D vector used for rendering.

typedef struct vector3F_t {
	float x;
	float y;
	float z;
} vector3F_t;

vector3F_t vector_add(vector3F_t a, vector3F_t b);

#endif // VECTOR3F_H
