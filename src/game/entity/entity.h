#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include "EntityAI.h"
#include "math/Box.h"
#include "math/Vector.h"

typedef struct EntityPhysics {
	Vector3D rotation;		
	Vector3D position;		// Current position of the entity is the game area.
	Vector3D velocity;		// Added to position every tick.
	Vector3D acceleration;	// Added to velocity every tick.
} EntityPhysics;

// Elliptical hitbox
typedef struct EntityHitbox {
	Vector3D focus1;
	Vector3D focus2;
	double distance;
} EntityHitbox;

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
	
	// If true, then the entity cannot be hit.
	bool invincible;
	
	// Tracks the last time invincibility frames were triggered by being hit.
	uint64_t iFrameTimer;
	
	/* Stats */
	int32_t currentHP;	// Current hit points of the entity.
	int32_t maxHP;		// Maximum hit points of the entity. Current HP cannot exceed this value.
	double speed;		// Maximum magnitude of velocity.
	
	/* Render */
	int32_t renderHandle; // Render object for the entity sprite.
	int32_t wireframe; // Quad index for the hitbox wireframe of the entity.
	
} Entity;

// Returns an entity with default values.
// Always use this function to initialize entities.
Entity new_entity(void);

// Ticks the entity's game logic.
void tick_entity(Entity *const pEntity);

// Triggers the invincibility frames of the entity if they are not already active.
void entityTriggerInvincibility(Entity *const pEntity);

bool entityCollision(const Entity e1, const Entity e2);

#endif	// ENTITY_H
