#include "EntityAI.h"

#include "entity.h"

#include "math/Vector.h"
#include "render/RenderManager.h"
#include "util/Random.h"
#include "util/time.h"

static void entityAIRegularTickNone(Entity *const pEntity) {
	(void)pEntity;
}

const EntityAI entityAINone = {
	.id = makeStaticString("none"),
	.onTick = entityAIRegularTickNone
};

static void entityAIRegularTickSlime(Entity *const pEntity) {
	
	//static const double speed = 0.125;
	static const double accelerationMagnitude = 0.24;
	
	const unsigned long long int currentTimeMS = getTimeMS();
	const unsigned long long int timeTillActionChange = random(0ULL, 2ULL) + random(1ULL, 2ULL);	// Seconds
	if ((currentTimeMS - pEntity->ai.lastActionTimeMS) / 1000ULL < timeTillActionChange) {
		pEntity->physics.acceleration = normVec(pEntity->physics.acceleration);
		pEntity->physics.acceleration = mulVec(pEntity->physics.acceleration, accelerationMagnitude);
		return;
	}
	
	const unsigned int currentAnimation = renderObjectGetAnimation(pEntity->renderHandle, 0);
	unsigned int nextAnimation = currentAnimation;
	
	const int nextDirection = random(0, 4);
	switch (nextDirection) {
		case 0: // NONE
			pEntity->physics.acceleration = (Vector3D){ 0.0, 0.0, 0.0 };
			nextAnimation = 0;
			break;
		case 1: // NORTH
			pEntity->physics.acceleration = (Vector3D){ 0.0, 1.0, 0.0 };
			nextAnimation = 1;
			break;
		case 2: // EAST
			pEntity->physics.acceleration = (Vector3D){ -1.0, 0.0, 0.0 };
			nextAnimation = 1;
			break;
		case 3: // SOUTH
			pEntity->physics.acceleration = (Vector3D){ 0.0, -1.0, 0.0 };
			nextAnimation = 1;
			break;
		case 4: // WEST
			pEntity->physics.acceleration = (Vector3D){ 1.0, 0.0, 0.0 };
			nextAnimation = 1;
			break;
	}
	pEntity->physics.acceleration = mulVec(pEntity->physics.acceleration, accelerationMagnitude);
	
	if (nextAnimation != currentAnimation) {
		renderObjectSetAnimation(pEntity->renderHandle, 0, nextAnimation);
	}
	
	pEntity->ai.lastActionTimeMS = currentTimeMS;
}

const EntityAI entityAISlime = {
	.id = makeStaticString("slime"),
	.onTick = entityAIRegularTickSlime
};