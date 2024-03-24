#ifndef ENTITY_H
#define ENTITY_H

#include "entity_ai.h"
#include "entity_transform.h"

#include "game/math/hitbox.h"
#include "render/render_object.h"

typedef struct entity_t {
	entity_transform_t transform;
	rect_t hitbox;
	entity_ai_t ai;
	render_handle_t render_handle;
} entity_t;

// Returns an entity with default values.
// Always use this function to initialize entities.
entity_t new_entity(void);

// Ticks the entity's game logic.
void tick_entity(entity_t *entity_ptr);

#endif	// ENTITY_H
