#include "entity_ai.h"

// "Null" or default AI behavior functions.

static void entityAIRegularTickNull(void) {
	
}

const EntityAI entityAINull = {
	.regularTick = entityAIRegularTickNull
};

static void entityAIRegularTickBasic(void) {
	
}

const EntityAI entityAIBasic = {
	.regularTick = entityAIRegularTickBasic
};