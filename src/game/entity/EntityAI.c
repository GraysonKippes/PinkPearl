#include "EntityAI.h"

#include "entity.h"
#include "math/Vector.h"
#include "render/RenderManager.h"
#include "util/Random.h"
#include "util/Time.h"

static void entityAIRegularTickNone(Entity *const pEntity) {
	(void)pEntity;
}

const EntityAI entityAINone = {
	.id = makeConstantString("none"),
	.onTick = entityAIRegularTickNone
};

static void entityAIRegularTickSlime(Entity *const pEntity) {
	
	static const double accelerationMagnitude = 0.24;
	
	const uint64_t currentTimeMS = getTimeMS();
	if (currentTimeMS - pEntity->ai.lastActionTimeMS >= pEntity->ai.timeTillNextAction) {
		pEntity->ai.lastActionTimeMS = currentTimeMS;
		pEntity->ai.timeTillNextAction = (random(0ULL, 3ULL) + random(1ULL, 4ULL)) * 1000ULL;
		pEntity->ai.direction = random(0, 4);
	}
	
	const unsigned int currentAnimation = renderObjectGetAnimation(pEntity->renderHandle, 0);
	unsigned int nextAnimation = currentAnimation;
	switch (pEntity->ai.direction) {
		case 0: // NONE
			pEntity->physics.acceleration = zeroVec3D;
			nextAnimation = 0;
			break;
		case 1: // NORTH
			pEntity->physics.acceleration = unitVec3DPosY;
			nextAnimation = 1;
			break;
		case 2: // EAST
			pEntity->physics.acceleration = unitVec3DNegX;
			nextAnimation = 1;
			break;
		case 3: // SOUTH
			pEntity->physics.acceleration = unitVec3DNegY;
			nextAnimation = 1;
			break;
		case 4: // WEST
			pEntity->physics.acceleration = unitVec3DPosX;
			nextAnimation = 1;
			break;
	}
	pEntity->physics.acceleration = mulVec(pEntity->physics.acceleration, accelerationMagnitude);
	if (nextAnimation != currentAnimation) {
		renderObjectSetAnimation(pEntity->renderHandle, 0, nextAnimation);
	}
}

const EntityAI entityAISlime = {
	.id = makeConstantString("slime"),
	.onTick = entityAIRegularTickSlime
};