#ifndef VECTOR3F_H
#define VECTOR3F_H

// Single-precision 3D vector used for rendering.

typedef struct vector3F_t {
	float x;
	float y;
	float z;
} vector3F_t;

extern const vector3F_t zero_vector3F;

vector3F_t vector3F_add(const vector3F_t a, const vector3F_t b);
vector3F_t vector3F_lerp(const vector3F_t start, const vector3F_t end, const double delta);

#endif // VECTOR3F_H
