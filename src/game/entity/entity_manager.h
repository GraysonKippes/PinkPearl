#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "entity.h"

#include "util/string.h"

#define MAX_NUM_ENTITIES 64

extern const int max_num_entities;

[[deprecated]]
typedef int entity_handle_t;

extern const entity_handle_t entityHandleInvalid;

// Initializes the entity manager by setting all entity slots to default values using `new_entity`.
void init_entity_manager(void);

// Frees the entity slot at the specified handle.
void unload_entity(const entity_handle_t handle);

// Loads an entity into the game world at the specified initial position and velocity.
// Returns a handle to the entity if entity loading succeeding, or an invalid handle if it failed.
int loadEntity(const String entityID, const vector3D_t initPosition, const vector3D_t initVelocity);

// Returns true if the specified entity handle is a valid entity handle and can be used to safely retrieve entities;
// 	returns false otherwise.
bool validateEntityHandle(const int entityHandle);

// Returns a pointer to the entity at the specified handle inside `entity_pptr`.
// Returns:
// 	0 if the retrieval was totally successful;
//	1 if `entity_pptr` is `NULL`;
//	2 if the entity handle is invalid according to `validate_entity_handle`; and
//	-1 if the retrieval was successful but came from an unused entity slot.
int getEntity(const entity_handle_t handle, entity_t **const ppEntity);

// Ticks the game logic of each loaded entity. Unused entity slots are skipped.
void tickEntities(void);

#endif	// ENTITY_MANAGER_H
