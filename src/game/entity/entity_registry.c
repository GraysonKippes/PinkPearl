#include "entity_registry.h"

#include <stddef.h>

#include "config.h"
#include "log/Logger.h"
#include "util/file_io.h"
#include "util/string.h"

#define FGE_PATH (RESOURCE_PATH "data/test.fge")

#define NUM_ENTITY_RECORDS 2
static const size_t num_entity_records = NUM_ENTITY_RECORDS;
static EntityRecord entity_records[NUM_ENTITY_RECORDS];

static bool register_entity_record(const EntityRecord entity_record);

void init_entity_registry(void) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Initializing entity registry...");
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		entity_records[i] = (EntityRecord){
			.entity_id = makeNullString(),
			.entity_hitbox = (BoxD){ },
			.textureID = makeNullString()
		};
	}
	
	FILE *fge_file = fopen(FGE_PATH, "rb");
	if (fge_file == nullptr) {
		return;
	}
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		
		EntityRecord entityRecord = { };
		
		// Read entity ID.
		entityRecord.entity_id = readString(fge_file, 32);
		file_next_block(fge_file);
		
		// Read entity hitbox.
		read_data(fge_file, 1, sizeof(BoxD), &entityRecord.entity_hitbox);
		
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
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		deleteString(&entity_records[i].entity_id);
		deleteString(&entity_records[i].textureID);
	}
	
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Done terminating entity registry.");
}

static bool register_entity_record(const EntityRecord entity_record) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Registering entity record with ID \"%s\"...", entity_record.entity_id.pBuffer);
	
	size_t hash_index = stringHash(entity_record.entity_id, num_entity_records);
	for (size_t i = 0; i < num_entity_records; ++i) {
		if (stringIsNull(entity_records[hash_index].entity_id)) {
			entity_records[hash_index] = entity_record;
			return true;
		}
		else {
			hash_index += 1;
			hash_index %= num_entity_records;
		}
	}
	return false;
}

bool find_entity_record(const String entity_id, EntityRecord *const pEntityRecord) {
	if (!pEntityRecord) {
		return false;
	}
	
	size_t hash_index = stringHash(entity_id, num_entity_records);
	for (size_t i = 0; i < num_entity_records; ++i) {
		if (stringCompare(entity_id, entity_records[i].entity_id)) {
			*pEntityRecord = entity_records[i];
			return true;
		}
		else {
			hash_index++;
			hash_index %= num_entity_records;
		}
	}
	return false;
}