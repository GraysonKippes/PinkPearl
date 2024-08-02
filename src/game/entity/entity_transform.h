#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

#include "math/vector3D.h"

typedef struct EntityTransform {
	Vector3D position;
	Vector3D velocity;
	unsigned long long int last_stationary_time;
} EntityTransform;

#endif	// ENTITY_TRANSFORM_H
