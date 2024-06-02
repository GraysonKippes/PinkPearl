#include "entity_manager.h"

#include <stddef.h>

#include "entity_registry.h"

#include "log/logging.h"
#include "render/render_object.h"
#include "render/renderer.h"

const int max_num_entities = MAX_NUM_ENTITIES;

static entity_t entities[MAX_NUM_ENTITIES];
static uint8_t entity_slot_enable_flags[MAX_NUM_ENTITIES];

const int entityHandleInvalid = -1;

void init_entity_manager(void) {
	for (int i = 0; i < max_num_entities; ++i) {
		entities[i] = new_entity();
		entity_slot_enable_flags[i] = 0;
	}
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

int loadEntity(const String entityID, const vector3D_t initPosition, const vector3D_t initVelocity) {
	if (stringIsNull(entityID)) {
		log_message(ERROR, "Error loading entity: string entityID is null.");
	}
	
	int entityHandle = entityHandleInvalid;
	for (int i = 0; validateEntityHandle(i); ++i) {
		if (!entity_slot_enable_flags[i]) {
			entity_slot_enable_flags[i] = 1;
			entityHandle = i;
		}
	}
	if (!validateEntityHandle(entityHandle)) {
		log_message(ERROR, "Error loading entity: failed to find available entity handle.");
		return entityHandleInvalid;
	}
	
	entity_record_t entityRecord = { 0 };
	if (!find_entity_record(entityID, &entityRecord)) {
		logf_message(ERROR, "Error loading entity: failed to find entity record with ID \"%s\".", entityID.buffer);
		return entityHandleInvalid;
	}
	
	const Transform transform = {
		.translation = (Vector4F){ (float)initPosition.x, (float)initPosition.y, (float)initPosition.z, 1.0F },
		.scaling = zeroVector4F,
		.rotation = zeroVector4F
	};
	
	//										  /*   Temporary parameter for testing   */
	const int renderHandle = loadRenderObject((DimensionsF){ -0.5F, 0.5F, 0.5F, -1.0F }, transform, entityRecord.entity_texture_id);
	if (!validateEntityHandle(renderHandle)) {
		log_message(ERROR, "Error loading entity: failed to load render object.");
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
