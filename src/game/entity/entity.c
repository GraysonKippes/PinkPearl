#include "entity.h"

#include <stddef.h>

#include "game/game.h"

#define SQUARE(x) (x * x)

entity_t new_entity(void) {
	return (entity_t){
		.transform = (entity_transform_t){ 0 },
		.hitbox = (rect_t){ 0 },
		.ai = entity_ai_null,
		.render_handle = render_handle_invalid
	};
}

void tick_entity(entity_t *entity_ptr) {

	if (entity_ptr == NULL) {
		return;
	}

	const vector3D_cubic_t old_position = entity_ptr->transform.position;
	const vector3D_cubic_t position_step = vector3D_spherical_to_cubic(entity_ptr->transform.velocity);

	vector3D_cubic_t new_position = vector3D_cubic_add(old_position, position_step);

	// The square of the distance of the currently selected new position from the old position.
	// This variable is used to track which resolved new position is the shortest from the entity.
	// The squared length is used instead of the real length because it is only used for comparison.
	double step_length_squared = SQUARE(position_step.x) + SQUARE(position_step.y) + SQUARE(position_step.z); 

	for (unsigned int i = 0; i < current_area.rooms[0].num_walls; ++i) {
		
		const rect_t wall = current_area.rooms[0].walls[i];

		vector3D_cubic_t resolved_position = resolve_collision(entity_ptr->transform, entity_ptr->hitbox, wall);
		vector3D_cubic_t resolved_step = vector3D_cubic_subtract(resolved_position, old_position);
		const double resolved_step_length_squared = SQUARE(resolved_step.x) + SQUARE(resolved_step.y) + SQUARE(resolved_step.z);

		if (resolved_step_length_squared < step_length_squared) {
			new_position = resolved_position;
			step_length_squared = resolved_step_length_squared;
		}
	}

	entity_ptr->transform.position = new_position;

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
