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

bool register_entity_record(const entity_record_t entity_record);

void init_entity_registry(void) {
	log_stack_push("EntityRegistry");
	log_message(VERBOSE, "Initializing entity registry...");
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		entity_records[i] = (entity_record_t){
			.entity_id = make_null_string(),
			.entity_hitbox = (rect_t){ 0.0, 0.0, 0.0, 0.0 },
			.entity_texture_id = make_null_string()
		};
	}
	
	FILE *fge_file = fopen(FGE_PATH, "rb");
	if (fge_file == NULL) {
		return;
	}
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		entity_record_t entity_record;
		entity_record.entity_id = read_string2(fge_file, 32);
		file_next_block(fge_file);
		read_data(fge_file, 1, sizeof(rect_t), &entity_record.entity_hitbox);
		entity_record.entity_texture_id = read_string2(fge_file, 32);
		file_next_block(fge_file);
		register_entity_record(entity_record);
	}
	
	fclose(fge_file);
	log_message(VERBOSE, "Done initializing entity registry.");
	log_stack_pop();
}

void terminate_entity_registry(void) {
	log_stack_push("EntityRegistry");
	log_message(VERBOSE, "Terminating entity registry...");
	
	for (size_t i = 0; i < num_entity_records; ++i) {
		destroy_string(&entity_records[i].entity_id);
		destroy_string(&entity_records[i].entity_texture_id);
	}
	
	log_message(VERBOSE, "Done terminating entity registry.");
	log_stack_pop();
}

bool register_entity_record(const entity_record_t entity_record) {
	size_t hash_index = string_hash(entity_record.entity_id, num_entity_records);
	for (size_t i = 0; i < num_entity_records; ++i) {
		if (is_string_null(entity_records[hash_index].entity_id)) {
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

bool find_entity_record(const string_t entity_id, entity_record_t *const entity_record_ptr) {
	if (entity_record_ptr == NULL) {
		return false;
	}
	
	size_t hash_index = string_hash(entity_id, num_entity_records);
	for (size_t i = 0; i < num_entity_records; ++i) {
		if (string_compare(entity_id, entity_records[i].entity_id)) {
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