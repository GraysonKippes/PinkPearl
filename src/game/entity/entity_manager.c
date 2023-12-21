#include "entity_manager.h"

#include <stddef.h>

const uint32_t max_num_entities = MAX_NUM_ENTITIES;

static entity_t entities[MAX_NUM_ENTITIES];

// Bitfield flags indicating which entity slots are loaded/occupied.
// One indicates that the corresponding entity slot is being used, zero indicates that it is not being used.
static uint64_t loaded_entity_slots = 0;

// Returns true if the bit at the corresponding position in the loaded entity slot flags is one;
// 	returns false otherwise.
static bool is_entity_slot_used(uint32_t slot) {
	return !!(loaded_entity_slots & (1LL << slot));
}

// Returns true if the bit at the corresponding position in the loaded entity slot flags is zero;
// 	returns false otherwise.
static bool is_entity_slot_unused(uint32_t slot) {
	return !(loaded_entity_slots & (1LL << slot));
}

static void set_entity_slot_used(uint32_t slot) {
	loaded_entity_slots |= (1LL << slot);
}

static void set_entity_slot_unused(uint32_t slot) {
	loaded_entity_slots &= ~(1LL << slot);
}

entity_handle_t load_entity(void) {
	
	// If all the bits in the bitfield are one, then there are no open entity slots left.
	// In this case, return maximum possible numeric value for the handle to indicate this error.
	if (loaded_entity_slots == UINT64_MAX) {
		return UINT32_MAX;
	}

	for (uint64_t i = 0; i < 64; ++i) {
		if (is_entity_slot_unused(i)) {
			set_entity_slot_used(i);
			return i;
		}
	}
	return UINT32_MAX;
}

void unload_entity(entity_handle_t handle) {
	
	if (validate_entity_handle(handle)) {
		set_entity_slot_unused(handle);
	}
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
