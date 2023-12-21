#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "entity.h"
#include "render/render_object.h"

#define MAX_NUM_ENTITIES 64

extern const uint32_t max_num_entities;

typedef uint32_t entity_handle_t;

extern const entity_handle_t entity_handle_invalid;

// Initializes the entity manager by setting all entity slots to default values.
void init_entity_manager(void);

// Returns a handle to the first available entity slot.
entity_handle_t load_entity(void);

// Frees the entity slot at the specified handle.
void unload_entity(entity_handle_t handle);

// Returns true if the specified entity handle is a valid entity handle and can be used to safely retrieve entities;
// 	returns false otherwise.
bool validate_entity_handle(entity_handle_t handle);

// Returns a pointer to the entity at the specified handle inside `entity_pptr`.
// Returns:
// 	0 if the retrieval was totally successful;
//	1 if `entity_pptr` is `NULL`;
//	2 if the entity handle is invalid according to `validate_entity_handle`; and
//	-1 if the retrieval was successful but came from an unused entity slot.
int get_entity_ptr(entity_handle_t handle, entity_t **entity_pptr);

// Ticks the game logic of each loaded entity. Unused entity slots are skipped.
void tick_entities(void);

#endif	// ENTITY_MANAGER_H
