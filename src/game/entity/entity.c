#include "entity.h"

#include <stddef.h>

entity_t new_entity(void) {
	return (entity_t){
		.transform = (entity_transform_t){ 0 },
		.hitbox = (hitbox_t){ 0 },
		.ai = entity_ai_null,
		.render_handle = render_handle_invalid
	};
}

void tick_entity(entity_t *entity_ptr) {

	if (entity_ptr == NULL) {
		return;
	}

	vector3D_cubic_t old_position = entity_ptr->transform.position;
	vector3D_cubic_t position_step = vector3D_spherical_to_cubic(entity_ptr->transform.velocity);
	entity_ptr->transform.position = vector3D_cubic_add(old_position, position_step);

	// Render object updates.

	const render_handle_t render_handle = entity_ptr->render_handle;

	// Make sure that the render handle of this entity goes to a valid entity render object.
	if (!validate_render_handle(render_handle) || is_render_handle_to_room_render_object(render_handle)) {
		return;
	}

	render_object_positions[render_handle].position.x = (float)entity_ptr->transform.position.x;
	render_object_positions[render_handle].position.y = (float)entity_ptr->transform.position.y;
	render_object_positions[render_handle].position.z = (float)entity_ptr->transform.position.z;
	render_object_positions[render_handle].previous_position.x = (float)old_position.x;
	render_object_positions[render_handle].previous_position.y = (float)old_position.y;
	render_object_positions[render_handle].previous_position.z = (float)old_position.z;
}
