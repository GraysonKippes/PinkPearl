#ifndef VECTOR4F_H
#define VECTOR4F_H

typedef struct vector4F_t {
	float x;
	float y;
	float z;
	float w;
} vector4F_t;

extern const vector4F_t vector4F_zero;

vector4F_t vector4F_add(const vector4F_t a, const vector4F_t b);

#endif // VECTOR4F_H
