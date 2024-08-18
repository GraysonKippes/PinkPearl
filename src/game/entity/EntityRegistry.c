#include "EntityRegistry.h"

#include <stddef.h>
#include <stdint.h>

#include "config.h"
#include "log/Logger.h"
#include "util/file_io.h"
#include "util/string.h"

#define FGE_PATH (RESOURCE_PATH "data/EntityRecordData.fge")

#define ENTITY_RECORD_COUNT 2

#define ENTITY_AI_COUNT 2

static const size_t entityRecordCount = ENTITY_RECORD_COUNT;

static EntityRecord entityRecords[ENTITY_RECORD_COUNT];

static const size_t entityAICount = ENTITY_AI_COUNT;

static EntityAI entityAIs[ENTITY_AI_COUNT];

static bool registerEntityRecord(const EntityRecord entityRecord);

static EntityAI findEntityAI(const String string);

void initEntityRegistry(void) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Initializing entity registry...");
	
	entityAIs[0] = entityAINone;
	entityAIs[1] = entityAISlime;
	
	for (size_t i = 0; i < entityRecordCount; ++i) {
		entityRecords[i] = (EntityRecord){
			.entityID = makeNullString(),
			.entityHitbox = (BoxD){ },
			.entityAI = entityAINone,
			.textureID = makeNullString(),
			.textureDimensions = (BoxF){ }
		};
	}
	
	FILE *fge_file = fopen(FGE_PATH, "rb");
	if (!fge_file) {
		return;
	}
	
	for (size_t i = 0; i < entityRecordCount; ++i) {
		
		EntityRecord entityRecord = { };
		
		// Read entity ID.
		entityRecord.entityID = readString(fge_file, 32);
		
		/* -- Entity Properties -- */
		
		String entityAIID = readString(fge_file, 32);	// Read entity AI ID.
		entityRecord.entityAI = findEntityAI(entityAIID);	// Find entity AI.
		deleteString(&entityAIID);
		read_data(fge_file, sizeof(BoxD), 1, &entityRecord.entityHitbox);	// Read entity hitbox.
		read_data(fge_file, 4, 1, &entityRecord.entityIsPersistent);	// Read entity persistency flag.
		
		/* -- Entity Statistics -- */
		
		read_data(fge_file, sizeof(int32_t), 1, &entityRecord.entityHP);	// Read maximum entity hitpoints.
		read_data(fge_file, sizeof(double), 1, &entityRecord.entitySpeed);	// Read maximum entity speed.
		
		/* -- Texture Properties -- */
		
		entityRecord.textureID = readString(fge_file, 32);	// Read entity texture ID.
		read_data(fge_file, sizeof(BoxF), 1, &entityRecord.textureDimensions); // Read entity texture dimensions.
		
		registerEntityRecord(entityRecord);
	}
	
	fclose(fge_file);
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Done initializing entity registry.");
}

void terminate_entity_registry(void) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Terminating entity registry...");
	
	for (size_t i = 0; i < entityRecordCount; ++i) {
		deleteString(&entityRecords[i].entityID);
		deleteString(&entityRecords[i].textureID);
	}
	
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Done terminating entity registry.");
}

static bool registerEntityRecord(const EntityRecord entityRecord) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Registering entity record with ID \"%s\"...", entityRecord.entityID.pBuffer);
	
	size_t hash_index = stringHash(entityRecord.entityID, entityRecordCount);
	for (size_t i = 0; i < entityRecordCount; ++i) {
		if (stringIsNull(entityRecords[hash_index].entityID)) {
			entityRecords[hash_index] = entityRecord;
			return true;
		}
		else {
			hash_index += 1;
			hash_index %= entityRecordCount;
		}
	}
	return false;
}

bool find_entity_record(const String entityID, EntityRecord *const pEntityRecord) {
	if (!pEntityRecord) {
		return false;
	}
	
	size_t hash_index = stringHash(entityID, entityRecordCount);
	for (size_t i = 0; i < entityRecordCount; ++i) {
		if (stringCompare(entityID, entityRecords[i].entityID)) {
			*pEntityRecord = entityRecords[i];
			return true;
		}
		else {
			hash_index++;
			hash_index %= entityRecordCount;
		}
	}
	return false;
}

static EntityAI findEntityAI(const String string) {
	if (stringIsNull(string)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Error finding entity AI: string ID is null.");
		return entityAINone;
	}
	
	// Just use a linear search to find the entity AI, probably faster than the hash function for low enough item count.
	for (size_t i = 0; i < entityAICount; ++i) {
		if (stringCompare(string, entityAIs[i].id)) {
			return entityAIs[i];
		}
	}
	
	logMsg(loggerGame, LOG_LEVEL_ERROR, "Error finding entity AI: no match found with ID \"%s\".", string.pBuffer);
	return entityAINone;
}