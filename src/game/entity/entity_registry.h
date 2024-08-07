#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

#include <stdbool.h>

#include "entity_ai.h"

#include "math/Box.h"
#include "util/string.h"

typedef struct EntityRecord {
	
	String entityID;
	
	BoxD entityHitbox;
	
	EntityAI entityAI;
	
	bool entityIsPersistent;
	
	int entityHP;
	
	double entitySpeed;
	
	String textureID;
	
	BoxF textureDimensions;
	
} EntityRecord;

void init_entity_registry(void);

void terminate_entity_registry(void);

// Searches for an existing entity record using its entity ID.
// Returns true if the entity record was found, which is stored in the pointer.
// Returns false if the entity record was not found, or if an error occurred.
bool find_entity_record(const String entityID, EntityRecord *const pEntityRecord);

#endif	// ENTITY_REGISTRY_H