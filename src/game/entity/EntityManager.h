#ifndef ENTITY_MANAGER_2_H
#define ENTITY_MANAGER_2_H

#include <stdint.h>
#include "math/Box.h"
#include "math/Vector.h"
#include "util/String.h"
#include "EntityAI.h"

typedef struct EntityComponentSystem_T *EntityComponentSystem;

typedef int32_t Entity2;

typedef void (*SystemFunctor)(EntityComponentSystem ecs, Entity2 entity);

typedef struct System {
	uint32_t componentMask;	// Specifies the components an entity must have at least to be matched by this system.
	SystemFunctor functor;	// The function that is called on each matching entity.
} System;

// Entity Components:
// EntityTraits: various flags that affect entity behavior or interaction with the game world. Generally immutable.
// EntityPhysics: gives entity physical traits that allow it to move and interact with the physical world.
// EntityAI: serves as the entity's brain, making decisions based on entity's surroundings.

typedef enum EntityComponent {
	ECMP_TRAITS		= 0x00000001U,
	ECMP_PHYSICS	= 0x00000002U,
	ECMP_HITBOX		= 0x00000004U,
	ECMP_HEALTH		= 0x00000008U,
	ECMP_AI			= 0x00000010U,
	ECMP_RENDER		= 0x00000020U
} EntityComponent;

typedef struct EntityTraits { // All intrinsic state.
	bool persistent;		// If true, entity is not unloaded between room transitions.
	bool airborne;			// If true, entity is not dragged by floor friction.
	bool harmfulToPlayer;	// If true, entity deals damage to the player upon contact.
	bool harmfulToEnemies;	// If true, entity deals damage to the player upon contact.
} EntityTraits;

typedef struct EntityPhysics {
	double maxSpeed;		// Maximum magnitude of velocity the entity can achieve.	// Intrinsic state.
	double mass;			// Mass of the entity. Force = mass * acceleration.			// Intrinsic state.
	Vector3D position;		// Current position of the entity is the game area.			// Extrinsic state.
	Vector3D velocity;		// Added to position every tick.							// Extrinsic state.
	Vector3D acceleration;	// Added to velocity every tick.							// Extrinsic state.
} EntityPhysics;

typedef struct EntityHealth {
	int32_t currentHP;		// Tracks the number of hit points the entity currently has.				// Extrinsic state.
	int32_t maxHP;			// Current hit points cannot normally exceed this value.					// Intrinsic state.
	bool invincible;		// Flipped to true when hit. True indicates I-frames are currently active.	// Extrinsic state.
	uint64_t iFrameTimer;	// Tracks the last time invincibility frames were triggered by being hit.	// Extrinsic state.
} EntityHealth;

typedef struct EntityRenderState {
	int32_t renderObjectHandle;
	int32_t wireframeHandle;
} EntityRenderState;

typedef struct EntityCreateInfo {
	uint32_t componentMask;
	EntityTraits traits;
	EntityPhysics physics;
	BoxD hitbox;
	EntityHealth health; // If currentHP is zero, then it is initialized to maxHP. Fields invincible and iFrameTimer are ignored.
	String textureID;
	BoxF textureDimensions;
} EntityCreateInfo;

// Creates and returns an entity component system.
EntityComponentSystem createEntityComponentSystem(void);

// Deletes the entity component system and replaces the value with null.
void deleteEntityComponentSystem(EntityComponentSystem *const pEntityComponentSystem);

// Creates an entity in the given entity component system and returns a handle to it.
Entity2 createEntity(EntityComponentSystem ecs, const EntityCreateInfo createInfo);

// Deletes an entity that was created in the given entity component system and resets the handle to null.
void deleteEntity(EntityComponentSystem ecs, Entity2 *const pEntity);

// Runs a system on all the matching entities in the entity component system.
void executeSystem(EntityComponentSystem ecs, System system);

#endif // ENTITY_MANAGER_2_H