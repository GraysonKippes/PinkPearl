#ifndef VECTOR4F_H
#define VECTOR4F_H

typedef struct Vector4F {
	float x;
	float y;
	float z;
	float w;
} Vector4F;

extern const Vector4F vector4F_zero;

Vector4F vector4F_add(const Vector4F a, const Vector4F b);

Vector4F vector4F_lerp(const Vector4F start, const Vector4F end, const float delta);

typedef struct Transform {
	Vector4F translation;
	Vector4F scaling;
	Vector4F rotation;
} Transform;

extern const Transform transformZero;

#endif // VECTOR4F_H
