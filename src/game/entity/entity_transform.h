#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

#include "math/vector3D.h"

typedef struct EntityTransform {
	Vector3D position;
	Vector3D velocity;
} EntityTransform;

#endif	// ENTITY_TRANSFORM_H
