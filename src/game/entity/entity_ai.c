#include "entity_ai.h"

#include "entity.h"

#include "math/vector3D.h"
#include "render/render_object.h"
#include "util/Random.h"
#include "util/time.h"

static void entityAIRegularTickNull(Entity *const pEntity) {
	(void)pEntity;
}

const EntityAI entityAINull = {
	.onTick = entityAIRegularTickNull
};

static void entityAIRegularTickSlime(Entity *const pEntity) {
	
	static const double speed = 0.125;
	
	const unsigned long long int currentTimeMS = getTimeMS();
	const unsigned long long int timeTillActionChange = random(0ULL, 2ULL) + random(1ULL, 2ULL);	// Seconds
	if ((currentTimeMS - pEntity->ai.lastActionTimeMS) / 1000ULL < timeTillActionChange) {
		return;
	}
	
	const unsigned int currentAnimation = renderObjectGetAnimation(pEntity->renderHandle, 0);
	unsigned int nextAnimation = currentAnimation;
	
	const int nextDirection = random(0, 4);
	switch (nextDirection) {
		case 0: // NONE
			pEntity->physics.velocity = (Vector3D){ 0.0, 0.0, 0.0 };
			nextAnimation = 0;
			break;
		case 1: // NORTH
			pEntity->physics.velocity = (Vector3D){ 0.0, 1.0, 0.0 };
			nextAnimation = 1;
			break;
		case 2: // EAST
			pEntity->physics.velocity = (Vector3D){ -1.0, 0.0, 0.0 };
			nextAnimation = 1;
			break;
		case 3: // SOUTH
			pEntity->physics.velocity = (Vector3D){ 0.0, -1.0, 0.0 };
			nextAnimation = 1;
			break;
		case 4: // WEST
			pEntity->physics.velocity = (Vector3D){ 1.0, 0.0, 0.0 };
			nextAnimation = 1;
			break;
	}
	pEntity->physics.velocity = vector3D_scalar_multiply(pEntity->physics.velocity, speed);
	
	if (nextAnimation != currentAnimation) {
		renderObjectSetAnimation(pEntity->renderHandle, 0, nextAnimation);
	}
	
	pEntity->ai.lastActionTimeMS = currentTimeMS;
}

const EntityAI entityAISlime = {
	.onTick = entityAIRegularTickSlime
};