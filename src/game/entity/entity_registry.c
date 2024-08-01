#include "entity_registry.h"

#include <stddef.h>

#include "config.h"
#include "log/logging.h"
#include "log/log_stack.h"
#include "util/file_io.h"
#include "util/string.h"

#define FGE_PATH (RESOURCE_PATH "data/test.fge")

#define NUM_ENTITY_RECORDS 2
static const size_t num_entity_records = NUM_ENTITY_RECORDS;
static entity_record_t entity_records[NUM_ENTITY_RECORDS];

static bool register_entity_record(const entity_record_t entity_record);

void init_entity_registry(void) {
	log_stack_push("EntityRegistry");
	logMsg(LOG_LEVEL_VERBOSE, "Initializing entity registry...");
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		entity_records[i] = (entity_record_t){
			.entity_id = makeNullString(),
			.entity_hitbox = (BoxD){ },
			.entity_texture_id = makeNullString()
		};
	}
	
	FILE *fge_file = fopen(FGE_PATH, "rb");
	if (fge_file == nullptr) {
		return;
	}
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		entity_record_t entity_record;
		entity_record.entity_id = readString(fge_file, 32);
		file_next_block(fge_file);
		read_data(fge_file, 1, sizeof(BoxD), &entity_record.entity_hitbox);
		entity_record.entity_texture_id = readString(fge_file, 32);
		file_next_block(fge_file);
		register_entity_record(entity_record);
	}
	
	fclose(fge_file);
	logMsg(LOG_LEVEL_VERBOSE, "Done initializing entity registry.");
	log_stack_pop();
}

void terminate_entity_registry(void) {
	log_stack_push("EntityRegistry");
	logMsg(LOG_LEVEL_VERBOSE, "Terminating entity registry...");
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		deleteString(&entity_records[i].entity_id);
		deleteString(&entity_records[i].entity_texture_id);
	}
	
	logMsg(LOG_LEVEL_VERBOSE, "Done terminating entity registry.");
	log_stack_pop();
}

static bool register_entity_record(const entity_record_t entity_record) {
	logMsgF(LOG_LEVEL_VERBOSE, "Registering entity record with ID \"%s\"...", entity_record.entity_id.buffer);
	
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

bool find_entity_record(const String entity_id, entity_record_t *const entity_record_ptr) {
	if (entity_record_ptr == nullptr) {
		return false;
	}
	
	size_t hash_index = stringHash(entity_id, num_entity_records);
	for (size_t i = 0; i < num_entity_records; ++i) {
		if (stringCompare(entity_id, entity_records[i].entity_id)) {
			*entity_record_ptr = entity_records[i];
			return true;
		}
		else {
			hash_index++;
			hash_index %= num_entity_records;
		}
	}
	return false;
}