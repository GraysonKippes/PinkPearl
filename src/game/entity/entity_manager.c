#include "entity_manager.h"

#include <stddef.h>

#include "EntityRegistry.h"

#include "log/Logger.h"
#include "render/render_object.h"
#include "render/renderer.h"

const int maxNumEntities = MAX_NUM_ENTITIES;

static Entity entities[MAX_NUM_ENTITIES];
static bool entitySlotEnabledFlags[MAX_NUM_ENTITIES];

const int entityHandleInvalid = -1;

void init_entity_manager(void) {
	for (int i = 0; i < maxNumEntities; ++i) {
		entities[i] = new_entity();
		entitySlotEnabledFlags[i] = false;
	}
}

int loadEntity(const String entityID, const Vector3D initPosition, const Vector3D initVelocity) {
	if (stringIsNull(entityID)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error loading entity: string entityID is null.");
	}
	
	int entityHandle = entityHandleInvalid;
	for (int i = 0; validateEntityHandle(i); ++i) {
		if (!entitySlotEnabledFlags[i]) {
			entityHandle = i;
			break;
		}
	}
	if (!validateEntityHandle(entityHandle)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error loading entity: failed to find available entity handle.");
		return entityHandleInvalid;
	}
	
	EntityRecord entityRecord = { };
	if (!find_entity_record(entityID, &entityRecord)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error loading entity: failed to find entity record with ID \"%s\".", entityID.pBuffer);
		return entityHandleInvalid;
	}
	
	const int renderHandle = loadRenderObject(entityRecord.textureID, entityRecord.textureDimensions, 1, &initPosition);
	if (!validateRenderObjectHandle(renderHandle)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error loading entity: failed to load render object.");
		return entityHandleInvalid;
	}
	
	// If debug mode is enabled, render the entity's hitbox.
	renderObjectLoadDebugQuad(renderHandle, initPosition, boxD2F(entityRecord.entityHitbox), (Vector4F){ 1.0F, 0.0F, 0.0F, 1.0F });
	
	entities[entityHandle].physics = (EntityPhysics){
		.position = initPosition,
		.velocity = initVelocity
	};
	entities[entityHandle].hitbox = entityRecord.entityHitbox;
	entities[entityHandle].ai = entityRecord.entityAI;
	entities[entityHandle].persistent = entityRecord.entityIsPersistent;
	entities[entityHandle].speed = entityRecord.entitySpeed;
	entities[entityHandle].renderHandle = renderHandle;
	
	entitySlotEnabledFlags[entityHandle] = true;
	
	return entityHandle;
}

void unloadEntity(const int entityHandle) {
	if (!validateEntityHandle(entityHandle)) {
		return;
	} else if (!entitySlotEnabledFlags[entityHandle]) {
		logMsg(loggerGame, LOG_LEVEL_WARNING, "Unloading already unused entity slot (%i).", entityHandle);
		return;
	}
	
	unloadRenderObject(&entities[entityHandle].renderHandle);
	entitySlotEnabledFlags[entityHandle] = false;
}

void unloadImpersistentEntities(void) {
	for (int i = 0; validateEntityHandle(i); ++i) {
		if (entitySlotEnabledFlags[i] && !entities[i].persistent) {
			unloadEntity(i);
		}
	}
}

bool validateEntityHandle(const int entityHandle) {
	return entityHandle >= 0 && entityHandle < maxNumEntities;
}

int getEntity(const int handle, Entity **const ppEntity) {
	if (!ppEntity) {
		return 1;
	} else if (!validateEntityHandle(handle)) {
		return 2;
	}

	*ppEntity = &entities[handle];

	// If the entity slot is unused, then still return the entity there, but return -1 as a warning.
	return entitySlotEnabledFlags[handle] ? 0 : -1;
}

void tickEntities(void) {
	for (int i = 0; i < maxNumEntities; ++i) {
		if (entitySlotEnabledFlags[i]) {
			tick_entity(&entities[i]);
		}
	}
}
