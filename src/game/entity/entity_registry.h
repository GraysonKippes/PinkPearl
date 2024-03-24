#ifndef ENTITY_REGISTRY_H
#define ENTITY_REGISTRY_H

#include <stdbool.h>

#include "game/math/hitbox.h"
#include "util/string.h"

typedef struct entity_record_t {
	string_t entity_id;
	rect_t entity_hitbox;
	string_t entity_texture_id;
} entity_record_t;

void init_entity_registry(void);
bool register_entity_record(const entity_record_t entity_record);

#endif	// ENTITY_REGISTRY_H