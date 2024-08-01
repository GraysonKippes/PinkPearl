#ifndef HITBOX_H
#define HITBOX_H

#include <stdbool.h>

#include "game/entity/entity_transform.h"
#include "Box.h"
#include "vector3D.h"

typedef struct hitbox_t {
	double width;
	double length;
} hitbox_t;

BoxD hitbox_to_world_space(hitbox_t hitbox, Vector3D position);

// Returns true if the two rects overlap; returns false otherwise.
bool rect_overlap(const BoxD a, const BoxD b);

Vector3D resolve_collision(const Vector3D old_position, const Vector3D new_position, const BoxD hitbox, const BoxD wall);

#endif	// HITBOX_H
