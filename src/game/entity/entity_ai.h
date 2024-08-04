#ifndef ENTITY_AI_H
#define ENTITY_AI_H

#include "util/string.h"

typedef struct Entity Entity;

typedef struct EntityAI {
	
	// TODO - add string-based ID for entity AI.
	
	// The time point in milliseconds of the last action performed by the entity.
	unsigned long long int lastActionTimeMS;

	// This function is called each tick, and it represents regular, idle behavior 
	// 	when there is no specific action being done.
	void (*onTick)(Entity *const pEntity);

} EntityAI;

// Entity AI that gives no behavior--AKA none of the behavior functions do anything.
extern const EntityAI entityAINull;

// The slime enemy's AI.
extern const EntityAI entityAISlime;

#endif	// ENTITY_AI_H
