#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "entity.h"
#include "render/render_object.h"

#define MAX_NUM_ENTITIES 64

extern const uint32_t max_num_entities;

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

// Ticks the game logic of each loaded entity. Unused entity slots are skipped.
void tick_entities(void);

#endif	// ENTITY_MANAGER_H
