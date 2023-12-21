#include "entity.h"

#include <stddef.h>

const uint32_t max_num_entities = MAX_NUM_ENTITIES;

static entity_t entities[MAX_NUM_ENTITIES];

// Bitfield flags indicating which entity slots are loaded/occupied.
// One indicates that the corresponding entity slot is being used, zero indicates that it is not being used.
static uint64_t loaded_entity_slots = 0;

entity_handle_t load_entity(void) {
	
	// If all the bits in the bitfield are one, then there are no open entity slots left.
	// In this case, return maximum possible numeric value for the handle to indicate this error.
	if (loaded_entity_slots == UINT64_MAX) {
		return UINT32_MAX;
	}

	for (uint64_t i = 0; i < 64; ++i) {
		if ((loaded_entity_slots & (1LL << i)) == 0) {
			loaded_entity_slots |= (1LL << i);
			return i;
		}
	}
	return UINT32_MAX;
}

void unload_entity(entity_handle_t handle) {
	
	if (handle >= max_num_entities || handle >= 64) {
		return;
	}

	loaded_entity_slots &= ~(1LL << handle);
}

bool validate_entity_handle(entity_handle_t handle) {
	return handle < max_num_entities && handle < 64;
}

entity_t *get_entity_ptr(entity_handle_t handle) {

	if (handle >= max_num_entities) {
		return NULL;
	}

	return entities + handle;
}
