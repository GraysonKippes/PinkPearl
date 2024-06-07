#ifndef ENTITY_SPAWNER_H
#define ENTITY_SPAWNER_H

#include "util/string.h"

typedef enum entity_spawner_reload_type_t {
	ENTITY_SPAWNER_RELOAD_NEVER = 0,			// Entity will never respawn.
	ENTITY_SPAWNER_RELOAD_AFTER_REFRESH = 1		// Entity will only respawn after room refresh.
} entity_spawner_reload_type_t;

typedef struct entity_spawner_t {
	
	// Indicates when/how often the entity is respawned.
	entity_spawner_reload_type_t reload_type;
	
	// Range of number of entities to spawn.
	int min_num_entities;
	int maxNumEntities;
	
	// String ID of the entity type to spawn.
	String entity_record_id;
	
} entity_spawner_t;

#endif	// ENTITY_SPAWNER_H