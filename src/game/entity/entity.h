#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include "EntityAI.h"
#include "math/Box.h"
#include "math/Vector.h"

typedef struct EntityPhysics {
	
	Vector3D position;
	
	Vector3D velocity;
	
	Vector3D acceleration;
	
} EntityPhysics;

// Represents a single "being" with the game (e.g. the player, NPCs, enemies, interactable objects).
typedef struct Entity {
	
	// The current position and velocity of the entity.
	EntityPhysics physics;
	
	// The hitbox, or collision bounds of the entity.
	BoxD hitbox;
	
	// The artificial intelligence of the entity, if it is a non-player mobile entity.
	EntityAI ai;
	
	// Controls whether or not this entity is unloaded when the player leaves the current room.
	bool persistent;
	
	/* Stats */
	
	int currentHP;
	
	int maxHP;
	
	double speed;
	
	// Handle to the render object associated with this entity.
	int32_t renderHandle;
	
} Entity;

// Returns an entity with default values.
// Always use this function to initialize entities.
Entity new_entity(void);

// Ticks the entity's game logic.
void tick_entity(Entity *const pEntity);

#endif	// ENTITY_H
