#include "entity.h"

#include <stddef.h>

void tick_entity(entity_t *entity_ptr) {

	if (entity_ptr == NULL) {
		return;
	}

	vector3D_cubic_t old_position = entity_ptr->transform.position;
	vector3D_cubic_t position_step = vector3D_spherical_to_cubic(entity_ptr->transform.velocity);
	entity_ptr->transform.position = vector3D_cubic_add(old_position, position_step);

	render_object_positions[entity_ptr->render_handle].position.x = (float)entity_ptr->transform.position.x;
	render_object_positions[entity_ptr->render_handle].position.y = (float)entity_ptr->transform.position.y;
	render_object_positions[entity_ptr->render_handle].position.z = (float)entity_ptr->transform.position.z;
	render_object_positions[entity_ptr->render_handle].previous_position.x = (float)old_position.x;
	render_object_positions[entity_ptr->render_handle].previous_position.y = (float)old_position.y;
	render_object_positions[entity_ptr->render_handle].previous_position.z = (float)old_position.z;
}
