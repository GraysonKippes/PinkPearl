#ifndef ENTITY_H
#define ENTITY_H

#include "entity_ai.h"
#include "entity_transform.h"

#include "math/Box.h"

typedef struct Entity {
	EntityTransform transform;
	BoxD hitbox;
	EntityAI ai;
	int render_handle;
} Entity;

// Returns an entity with default values.
// Always use this function to initialize entities.
Entity new_entity(void);

// Ticks the entity's game logic.
void tick_entity(Entity *pEntity);

#endif	// ENTITY_H
