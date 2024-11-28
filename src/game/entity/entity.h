#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include "EntityAI.h"
#include "math/Box.h"
#include "math/Vector.h"

typedef struct EntityPhysics {
	Vector3D position;		// Current position of the entity is the game area.
	Vector3D velocity;		// Added to position every tick.
	Vector3D acceleration;	// Added to velocity every tick.
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
	int32_t currentHP;	// Current hit points of the entity.
	int32_t maxHP;		// Maximum hit points of the entity. Current HP cannot exceed this value.
	double speed;		// Maximum magnitude of velocity.
	
	// Handle to the render object associated with this entity.
	int32_t renderHandle;
	
} Entity;

// Returns an entity with default values.
// Always use this function to initialize entities.
Entity new_entity(void);

// Ticks the entity's game logic.
void tick_entity(Entity *const pEntity);

#endif	// ENTITY_H
