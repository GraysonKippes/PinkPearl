#ifndef HITBOX_H
#define HITBOX_H

#include "vector3D.h"

typedef struct hitbox_t {
	double width;
	double length;
} hitbox_t;

hitbox_t hitbox_to_world_space(hitbox_t hitbox, vector3D_cubic_t position);

#endif	// HITBOX_H
