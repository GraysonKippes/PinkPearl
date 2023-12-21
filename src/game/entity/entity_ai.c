#include "entity_ai.h"

// "Null" or default AI behavior functions.

static void entity_ai_regular_tick_null(void) {
	
}

const entity_ai_t entity_ai_null = {
	.regular_tick = entity_ai_regular_tick_null
};
