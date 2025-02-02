#ifndef ENTITY_AI_H
#define ENTITY_AI_H

#include <stdint.h>
#include "util/String.h"

typedef struct Entity Entity;

typedef struct EntityAI {
	
	// The unique, string-based identifier for this entity AI.
	String id;
	
	// The time point in milliseconds of the last action performed by the entity.
	uint64_t lastActionTimeMS;
	uint64_t timeTillNextAction;
	
	uint32_t direction;

	// This function is called each tick, and it represents regular, idle behavior 
	// 	when there is no specific action being done.
	void (*onTick)(Entity *const pEntity);

} EntityAI;

// Entity AI that gives no behavior--AKA none of the behavior functions do anything.
extern const EntityAI entityAINone;

// The slime enemy's AI.
extern const EntityAI entityAISlime;

#endif	// ENTITY_AI_H
