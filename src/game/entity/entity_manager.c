#include "entity_manager.h"

#include <stddef.h>

const uint32_t max_num_entities = MAX_NUM_ENTITIES;

static entity_t entities[MAX_NUM_ENTITIES];

const entity_handle_t entity_handle_invalid = UINT32_MAX;

// Bitfield flags indicating which entity slots are loaded/occupied.
// One indicates that the corresponding entity slot is being used, zero indicates that it is not being used.
static uint64_t loaded_entity_slots = 0;

// Returns true if the bit at the corresponding position in the loaded entity slot flags is one;
// 	returns false otherwise.
static bool is_entity_slot_used(uint64_t slot) {
	return !!(loaded_entity_slots & (1LL << slot));
}

// Returns true if the bit at the corresponding position in the loaded entity slot flags is zero;
// 	returns false otherwise.
static bool is_entity_slot_unused(uint64_t slot) {
	return !(loaded_entity_slots & (1LL << slot));
}

static void set_entity_slot_used(uint64_t slot) {
	loaded_entity_slots |= (1LL << slot);
}

static void set_entity_slot_unused(uint64_t slot) {
	loaded_entity_slots &= ~(1LL << slot);
}

void init_entity_manager(void) {
	
	loaded_entity_slots = 0;

	for (uint32_t i = 0; i < max_num_entities; ++i) {
		entities[i].transform = (entity_transform_t){ 0 };
		entities[i].ai = entity_ai_null;
		entities[i].render_handle = render_handle_invalid;
	}
}

entity_handle_t load_entity(void) {
	
	// If all the bits in the bitfield are one, then there are no open entity slots left.
	// In this case, return maximum possible numeric value for the handle to indicate this error.
	if (loaded_entity_slots == UINT64_MAX) {
		return entity_handle_invalid;
	}

	for (uint64_t i = 0; i < 64; ++i) {
		if (is_entity_slot_unused(i)) {
			set_entity_slot_used(i);
			return i;
		}
	}
	return entity_handle_invalid;
}

void unload_entity(entity_handle_t handle) {
	
	if (validate_entity_handle(handle)) {
		set_entity_slot_unused(handle);
	}
}

bool validate_entity_handle(entity_handle_t handle) {
	return handle < max_num_entities && handle < 64;
}

int get_entity_ptr(entity_handle_t handle, entity_t **entity_pptr) {
	
	if (entity_pptr == NULL) {
		return 1;
	}

	if (!validate_entity_handle(handle)) {
		return 2;
	}

	*entity_pptr = entities + handle;

	// If the entity slot is unused, then still return the entity there, but return -1 as a warning.
	return is_entity_slot_used(handle) ? 0 : -1;
}

void tick_entities(void) {
	
	// If none of the entity slots are currently used, then just skip the entire loop.
	if (loaded_entity_slots == 0) {
		return;
	}

	for (uint32_t i = 0; validate_entity_handle(i); ++i) {
		if (is_entity_slot_used(i)) {
			tick_entity(entities + i);
		}
	}
}
