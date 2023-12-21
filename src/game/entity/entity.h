#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include <stdint.h>

#include "entity_transform.h"

#define MAX_NUM_ENTITIES 64

extern const uint32_t max_num_entities;

typedef struct entity_t {

	entity_transform_t transform;


} entity_t;

typedef uint32_t entity_handle_t;

// Returns a handle to the first available entity slot.
entity_handle_t load_entity(void);

// Frees the entity slot at the specified handle.
void unload_entity(entity_handle_t handle);

// Returns true if the specified entity handle is a valid entity handle and can be used to safely retrieve entities;
// 	returns false otherwise.
bool validate_entity_handle(entity_handle_t handle);

// Returns a pointer to the entity at the specified handle, or NULL if such an entity does not exist.
entity_t *get_entity_ptr(entity_handle_t handle);

#endif	// ENTITY_H
