#ifndef HITBOX_H
#define HITBOX_H

#include <stdbool.h>

#include "game/entity/entity_transform.h"
#include "vector3D.h"

typedef struct hitbox_t {
	double width;
	double length;
} hitbox_t;

typedef struct rect_t {
	// Position 2 should be greater than position 1.
	double x1, y1;	// Bottom-left corner of the rect.
	double x2, y2;	// Top-right corner of the rect.
} rect_t;

rect_t hitbox_to_world_space(hitbox_t hitbox, vector3D_cubic_t position);

// Returns true if the two rects overlap; returns false otherwise.
bool rect_overlap(const rect_t a, const rect_t b);

vector3D_cubic_t resolve_collision(const vector3D_cubic_t old_position, const vector3D_cubic_t new_position, const rect_t hitbox, const rect_t wall);

#endif	// HITBOX_H
