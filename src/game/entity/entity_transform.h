#ifndef ENTITY_TRANSFORM_H
#define ENTITY_TRANSFORM_H

#include <stdint.h>

#include "game/math/vector3D.h"

typedef struct entity_transform_t {

	vector3D_cubic_t position;

	vector3D_spherical_t velocity;

	uint64_t last_stationary_time;

} entity_transform_t;

#endif	// ENTITY_TRANSFORM_H
