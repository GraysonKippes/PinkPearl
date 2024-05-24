#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

#include "math/vector3D.h"

typedef struct entity_transform_t {
	vector3D_t position;
	vector3D_t velocity;
	unsigned long long int last_stationary_time;
} entity_transform_t;

#endif	// ENTITY_TRANSFORM_H
