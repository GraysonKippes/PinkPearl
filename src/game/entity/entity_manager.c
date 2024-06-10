#include "entity_manager.h"

#include <stddef.h>

#include "entity_registry.h"

#include "log/logging.h"
#include "render/render_object.h"
#include "render/renderer.h"

const int maxNumEntities = MAX_NUM_ENTITIES;

static entity_t entities[MAX_NUM_ENTITIES];
static uint8_t entity_slot_enable_flags[MAX_NUM_ENTITIES];

const int entityHandleInvalid = -1;

void init_entity_manager(void) {
	for (int i = 0; i < maxNumEntities; ++i) {
		entities[i] = new_entity();
		entity_slot_enable_flags[i] = 0;
	}
}

void unload_entity(const int handle) {
	if (!validateEntityHandle(handle)) {
		return;
	} else if (entity_slot_enable_flags[handle] == 0) {
		logMsgF(WARNING, "Unloading already unused entity slot (%u).", handle);
		return;
	}
	
	entity_slot_enable_flags[handle] = 0;
}

int loadEntity(const String entityID, const Vector3D initPosition, const Vector3D initVelocity) {
	if (stringIsNull(entityID)) {
		logMsg(ERROR, "Error loading entity: string entityID is null.");
	}
	
	int entityHandle = entityHandleInvalid;
	for (int i = 0; validateEntityHandle(i); ++i) {
		if (!entity_slot_enable_flags[i]) {
			entity_slot_enable_flags[i] = 1;
			entityHandle = i;
		}
	}
	if (!validateEntityHandle(entityHandle)) {
		logMsg(ERROR, "Error loading entity: failed to find available entity handle.");
		return entityHandleInvalid;
	}
	
	entity_record_t entityRecord = { };
	if (!find_entity_record(entityID, &entityRecord)) {
		logMsgF(ERROR, "Error loading entity: failed to find entity record with ID \"%s\".", entityID.buffer);
		return entityHandleInvalid;
	}
	
	//										  /*   Temporary parameter for testing   */
	const int renderHandle = loadRenderObject(entityRecord.entity_texture_id, (DimensionsF){ -0.5F, -1.0F, 0.5F, 0.5F }, 1, &initPosition);
	if (!validateEntityHandle(renderHandle)) {
		logMsg(ERROR, "Error loading entity: failed to load render object.");
		return entityHandleInvalid;
	}
	
	entities[entityHandle].transform = (entity_transform_t){
		.position = initPosition,
		.velocity = initVelocity
	};
	entities[entityHandle].hitbox = entityRecord.entity_hitbox;
	entities[entityHandle].ai = entityAINull;
	entities[entityHandle].render_handle = renderHandle;
	
	return entityHandle;
}

bool validateEntityHandle(const int entityHandle) {
	return entityHandle >= 0 && entityHandle < maxNumEntities;
}

int getEntity(const int handle, entity_t **const ppEntity) {
	if (ppEntity == nullptr) {
		return 1;
	} else if (!validateEntityHandle(handle)) {
		return 2;
	}

	*ppEntity = entities + handle;

	// If the entity slot is unused, then still return the entity there, but return -1 as a warning.
	return entity_slot_enable_flags[handle] ? 0 : -1;
}

void tickEntities(void) {
	for (int i = 0; i < maxNumEntities; ++i) {
		if (entity_slot_enable_flags[i]) {
			tick_entity(&entities[i]);
		}
	}
}
