#include "entity_registry.h"

#include <stddef.h>

#include "config.h"
#include "log/Logger.h"
#include "util/file_io.h"
#include "util/string.h"

#define FGE_PATH (RESOURCE_PATH "data/test.fge")

#define ENTITY_RECORD_COUNT 3

static const size_t entityRecordCount = ENTITY_RECORD_COUNT;

static EntityRecord entityRecords[ENTITY_RECORD_COUNT];

static bool register_entity_record(const EntityRecord entityRecord);

void init_entity_registry(void) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Initializing entity registry...");
	
	for (size_t i = 0; i < entityRecordCount; ++i) {
		entityRecords[i] = (EntityRecord){
			.entityID = makeNullString(),
			.entityHitbox = (BoxD){ },
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
		file_next_block(fge_file);
		
		// Read entity hitbox.
		read_data(fge_file, 1, sizeof(BoxD), &entityRecord.entityHitbox);
		
		// Read entity texture ID.
		entityRecord.textureID = readString(fge_file, 32);
		file_next_block(fge_file);
		
		// Read entity texture dimensions.
		read_data(fge_file, 1, sizeof(BoxF), &entityRecord.textureDimensions);
		
		// Read entity properties.
		read_data(fge_file, 1, 1, &entityRecord.entityIsPersistent);
		
		file_next_block(fge_file);
		
		register_entity_record(entityRecord);
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

static bool register_entity_record(const EntityRecord entityRecord) {
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