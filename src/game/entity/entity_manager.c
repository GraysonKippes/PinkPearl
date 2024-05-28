#include "entity_manager.h"

#include <stddef.h>

#include "log/logging.h"

const int max_num_entities = MAX_NUM_ENTITIES;

static entity_t entities[MAX_NUM_ENTITIES];
static uint8_t entity_slot_enable_flags[MAX_NUM_ENTITIES];

const entity_handle_t entity_handle_invalid = UINT32_MAX;

void init_entity_manager(void) {
	for (int i = 0; i < max_num_entities; ++i) {
		entities[i] = new_entity();
		entity_slot_enable_flags[i] = 0;
	}
}

entity_handle_t load_entity(void) {
	// Loop through all the entity slots and find the first one available.
	// TODO - replace with (priority?) queue
	for (int i = 0; validateEntityHandle(i); ++i) {
		if (!entity_slot_enable_flags[i]) {
			entity_slot_enable_flags[i] = 1;
			return i;
		}
	}
	return entity_handle_invalid;
}

void unload_entity(const entity_handle_t handle) {
	if (!validateEntityHandle(handle)) {
		return;
	} else if (entity_slot_enable_flags[handle] == 0) {
		logf_message(WARNING, "Unloading already unused entity slot (%u).", handle);
		return;
	}
	
	entity_slot_enable_flags[handle] = 0;
}

bool validateEntityHandle(const int entityHandle) {
	return entityHandle >= 0 && entityHandle < max_num_entities;
}

int getEntity(const entity_handle_t handle, entity_t **const ppEntity) {
	if (ppEntity == NULL) {
		return 1;
	} else if (!validateEntityHandle(handle)) {
		return 2;
	}

	*ppEntity = entities + handle;

	// If the entity slot is unused, then still return the entity there, but return -1 as a warning.
	return entity_slot_enable_flags[handle] ? 0 : -1;
}

void tickEntities(void) {
	for (int i = 0; i < max_num_entities; ++i) {
		if (entity_slot_enable_flags[i]) {
			tick_entity(&entities[i]);
		}
	}
}
