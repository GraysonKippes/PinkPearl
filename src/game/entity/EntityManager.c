#include "EntityManager.h"

#include <assert.h>
#include <stdlib.h>
#include "log/Logger.h"
#include "render/RenderManager.h"
#include "util/Allocation.h"
//#include "EntityAI.h"

#define ECS_ELEMENT_COUNT	64
#define MAX_ENTITY_COUNT 	(ECS_ELEMENT_COUNT - 1)

// Requirements of entity component system:
//	* Systems must operate on all entities with *at least* all requirement components.
//	* Each component must reference back to the entity that it "owns" it.
//	* Each entity must have a bitmask indicating which components it has.
//	* Entity handle zero must be reserved to indicate a null entity.

struct EntityComponentSystem_T {
	
	int32_t stackPosition; // Index of the top element in the stack.
	Entity entityStack[MAX_ENTITY_COUNT];
	
	bool slots[ECS_ELEMENT_COUNT];
	uint32_t componentMasks[ECS_ELEMENT_COUNT];
	
	EntityTraits 		traits[ECS_ELEMENT_COUNT];
	EntityPhysics 		physics[ECS_ELEMENT_COUNT];
	BoxD 				hitboxes[ECS_ELEMENT_COUNT];
	EntityHealth 		healths[ECS_ELEMENT_COUNT];
	EntityRenderState	renderStates[ECS_ELEMENT_COUNT];
};

EntityComponentSystem createEntityComponentSystem(void) {
	EntityComponentSystem ecs = heapAlloc(1, sizeof(struct EntityComponentSystem_T));
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity component system: failed to allocate entity component system.");
		return nullptr;
	}
	ecs->stackPosition = MAX_ENTITY_COUNT - 1;
	for (int32_t i = 0; i < MAX_ENTITY_COUNT; ++i) {
		ecs->entityStack[i] = (Entity)(i + 1);
	}
	return ecs;
}

void deleteEntityComponentSystem(EntityComponentSystem *const pEntityComponentSystem) {
	assert(pEntityComponentSystem);
	*pEntityComponentSystem = heapFree(*pEntityComponentSystem);
}

Entity createEntity(EntityComponentSystem ecs, const EntityCreateInfo createInfo) {
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity: entity component system is null.");
		return (Entity){ };
	}
	
	if (createInfo.componentMask == 0) {
		logMsg(loggerGame, LOG_LEVEL_WARNING, "Creating entity: component mask is zero.");
	}

	Entity entity = { };
	if (ecs->stackPosition > 0) {
		entity = ecs->entityStack[ecs->stackPosition--];
	} else {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity: no entity slots available in entity component system %p.", ecs);
		return (Entity){ };
	}
	
	ecs->slots[entity] = true;
	ecs->componentMasks[entity] = createInfo.componentMask;
	
	if (createInfo.componentMask & ECMP_TRAITS) {
		ecs->traits[entity] = createInfo.traits;
	}
	
	if (createInfo.componentMask & ECMP_PHYSICS) {
		ecs->physics[entity] = createInfo.physics;
	}
	
	if (createInfo.componentMask & ECMP_HITBOX) {
		ecs->hitboxes[entity] = createInfo.hitbox;
	}
	
	if (createInfo.componentMask & ECMP_HEALTH) {
		ecs->healths[entity] = (EntityHealth){
			.currentHP = createInfo.health.currentHP == 0 ? createInfo.health.maxHP : createInfo.health.currentHP,
			.maxHP = createInfo.health.maxHP,
			.invincible = false,
			.iFrameTimer = 0
		};
	}
	
	// EntityRenderState requires EntityPhysics and hitbox
	// TODO: make hitbox optional, wireframe is only rendered if there is a hitbox.
	if (createInfo.componentMask & (ECMP_PHYSICS | ECMP_HITBOX | ECMP_RENDER)) {
		const RenderObjectLoadInfo renderObjectLoadInfo = {
			.textureID = createInfo.textureID,
			.quadCount = 1, // TODO: load wireframe if debug menu is enabled.
			.pQuadLoadInfos = (QuadLoadInfo[2]){
				{
					.quadType = QUAD_TYPE_MAIN,
					.initPosition = createInfo.physics.position,
					.quadDimensions = createInfo.textureDimensions,
					.initAnimation = 2,
					.color = COLOR_WHITE
				}, {
					.quadType = QUAD_TYPE_WIREFRAME,
					.initPosition = createInfo.physics.position,
					.quadDimensions = boxD2F(createInfo.hitbox),
					.initAnimation = 0,
					.color = COLOR_RED // TODO: select color green if entity is the player.
				}
			}
		};
		ecs->renderStates[entity].renderObjectHandle = loadRenderObject(renderObjectLoadInfo);
		if (!renderObjectExists(ecs->renderStates[entity].renderObjectHandle)) {
			logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity: failed to load render object.");
		}
		ecs->renderStates[entity].wireframeHandle = -1;
	}
	
	return entity;
}

void deleteEntity(EntityComponentSystem ecs, Entity *const pEntity) {
	assert(pEntity);
	
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: entity component system is null.");
		return;
	}
	
	if (*pEntity < 1 || *pEntity >= ECS_ELEMENT_COUNT) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: entity handle %i is invalid.", *pEntity);
		return;
	} else if (ecs->slots[*pEntity]) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: entity %i does not exist.", *pEntity);
		return;
	}
	
	if (ecs->stackPosition >= 0 && (ecs->stackPosition + 1) < MAX_ENTITY_COUNT) {
		ecs->slots[*pEntity] = false;
		ecs->componentMasks[*pEntity] = 0U;
		ecs->entityStack[++ecs->stackPosition] = *pEntity;
		*pEntity = (Entity){ };
	} else {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: stack position %i in entity component system %p is invalid.", ecs->stackPosition, ecs);
	}
}