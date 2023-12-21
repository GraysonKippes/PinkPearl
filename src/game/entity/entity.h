#ifndef ENTITY_H
#define ENTITY_H

#include "entity_transform.h"
#include "render/render_object.h"

typedef struct entity_t {

	entity_transform_t transform;

	render_handle_t render_handle;

} entity_t;

void tick_entity(entity_t *entity_ptr);

#endif	// ENTITY_H
