#include "entity.h"

#include <stddef.h>

#include "game/game.h"
#include "render/render_object.h"
#include "render/vulkan/math/render_vector.h"

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

	const vector3D_t old_position = entity_ptr->transform.position;
	const vector3D_t position_step = entity_ptr->transform.velocity;
	vector3D_t new_position = vector3D_add(old_position, position_step);

	// The square of the distance of the currently selected new position from the old position.
	// This variable is used to track which resolved new position is the shortest from the entity.
	// The squared length is used instead of the real length because it is only used for comparison.
	double step_length_squared = SQUARE(position_step.x) + SQUARE(position_step.y) + SQUARE(position_step.z); 

	for (unsigned int i = 0; i < current_area.rooms[0].num_walls; ++i) {
		
		const rect_t wall = current_area.rooms[0].walls[i];

		vector3D_t resolved_position = resolve_collision(old_position, new_position, entity_ptr->hitbox, wall);
		vector3D_t resolved_step = vector3D_subtract(resolved_position, old_position);
		const double resolved_step_length_squared = SQUARE(resolved_step.x) + SQUARE(resolved_step.y) + SQUARE(resolved_step.z);

		if (resolved_step_length_squared < step_length_squared) {
			new_position = resolved_position;
			step_length_squared = resolved_step_length_squared;
		}
	}

	entity_ptr->transform.position = new_position;

	// Update render object.
	const int render_handle = entity_ptr->render_handle;
	if (!validateRenderHandle(render_handle)) {
		return;
	}
	const Vector4F render_position = {
		.x = (float)entity_ptr->transform.position.x,
		.y = (float)entity_ptr->transform.position.y,
		.z = (float)entity_ptr->transform.position.z,
		.w = 1.0F
	};
	render_vector_set(&getRenderObjTransform(render_handle)->translation, render_position);
}
