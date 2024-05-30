#ifndef ENTITY_AI_H
#define ENTITY_AI_H

typedef struct entity_ai_t {

	// This function is called each tick, and it represents regular, idle behavior 
	// 	when there is no specific action being done.
	void (*regular_tick)(void);

} entity_ai_t;

// Entity AI that gives no behavior--AKA none of the behavior functions do anything.
extern const entity_ai_t entityAINull;

#endif	// ENTITY_AI_H
