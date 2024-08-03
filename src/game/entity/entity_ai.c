#include "entity_ai.h"

#include "entity.h"

#include "util/Random.h"

static void entityAIRegularTickNull(Entity *const pEntity) {
	(void)pEntity;
}

const EntityAI entityAINull = {
	.onTick = entityAIRegularTickNull
};

static void entityAIRegularTickSlime(Entity *const pEntity) {
	
	static const double speed = 0.125;
	
	pEntity->transform.velocity.x = speed;
}

const EntityAI entityAISlime = {
	.onTick = entityAIRegularTickSlime
};