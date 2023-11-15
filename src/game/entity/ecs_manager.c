#include "ecs_manager.h"

#include <tECS/tECS.h>

#include "render/render_object.h"

component_index_t component_index_render_handle;

static archetype_t archetype_mob;

void init_tECS(void) {
	register_component_type(render_handle_t, &component_index_render_handle);
	create_archetype(0x01, &archetype_mob);
	init_entity_manager();
}
