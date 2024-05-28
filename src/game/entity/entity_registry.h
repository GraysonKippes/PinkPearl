#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

#include <stdbool.h>

#include "math/hitbox.h"
#include "util/string.h"

typedef struct entity_record_t {
	String entity_id;
	rect_t entity_hitbox;
	String entity_texture_id;
} entity_record_t;

void init_entity_registry(void);
void terminate_entity_registry(void);

// Searches for an existing entity record using its entity ID.
// Returns true if the entity record was found, which is stored in the pointer.
// Returns false if the entity record was not found, or if an error occurred.
bool find_entity_record(const String entity_id, entity_record_t *const entity_record_ptr);

#endif	// ENTITY_REGISTRY_H