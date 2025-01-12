#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

#include <stdint.h>
#include "EntityAI.h"
#include "math/Box.h"
#include "util/String.h"

typedef struct EntityRecord {
	
	String entityID;
	
	/* -- Entity Properties -- */
	
	EntityAI entityAI;
	
	BoxD entityHitbox;
	
	bool entityIsPersistent;
	
	/* -- Entity Statistics -- */
	
	int32_t entityHP;
	
	double entitySpeed;
	
	/* -- Texture Properties -- */
	
	String textureID;
	
	BoxF textureDimensions;
	
} EntityRecord;

void initEntityRegistry(void);

void terminate_entity_registry(void);

// Searches for an existing entity record using its entity ID.
// Returns true if the entity record was found, which is stored in the pointer.
// Returns false if the entity record was not found, or if an error occurred.
bool find_entity_record(const String entityID, EntityRecord *const pEntityRecord);

#endif	// ENTITY_REGISTRY_H