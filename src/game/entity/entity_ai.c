#include "entity_ai.h"

#include "entity.h"

static void entityAIRegularTickNull(Entity *const pEntity) {
	(void)pEntity;
}

const EntityAI entityAINull = {
	.regularTick = entityAIRegularTickNull
};

static void entityAIRegularTickSlime(Entity *const pEntity) {
	pEntity->transform.velocity.x = -0.125;
}

const EntityAI entityAISlime = {
	.regularTick = entityAIRegularTickSlime
};