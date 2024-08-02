#ifndef ENTITY_AI_H
#define ENTITY_AI_H

typedef struct EntityAI {

	// This function is called each tick, and it represents regular, idle behavior 
	// 	when there is no specific action being done.
	void (*regularTick)(void);

} EntityAI;

// Entity AI that gives no behavior--AKA none of the behavior functions do anything.
extern const EntityAI entityAINull;

extern const EntityAI entityAIBasic;

#endif	// ENTITY_AI_H
