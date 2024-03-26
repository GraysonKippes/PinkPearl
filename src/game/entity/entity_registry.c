#include "entity_registry.h"

#include <stddef.h>

#define NUM_ENTITY_RECORDS 1
static const size_t num_entity_records = NUM_ENTITY_RECORDS;
static entity_record_t entity_records[NUM_ENTITY_RECORDS];

void init_entity_registry(void) {
	for (size_t i = 0; i < num_entity_records; ++i) {
		entity_records[i] = (entity_record_t){
			.entity_id = make_null_string(),
			.entity_hitbox = (rect_t){ 0.0 },
			.entity_texture_id = make_null_string()
		};
	}
}

bool register_entity_record(const entity_record_t entity_record) {
	size_t hash_index = string_hash(entity_record.entity_id, num_entity_records);
	for (size_t i = 0; i < num_entity_records; ++i) {
		if (is_string_null(entity_records[hash_index].entity_id)) {
			entity_records[hash_index] = entity_record;
			return true;
		}
		else {
			hash_index++;
			hash_index %= num_entity_records;
		}
	}
	return false;
}

bool find_entity_record(const string_t entity_id, entity_record_t *const entity_record_ptr) {
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