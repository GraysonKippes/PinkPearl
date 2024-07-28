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
		.ai = entityAINull,
		.render_handle = renderHandleInvalid
	};
}

void tick_entity(entity_t *pEntity) {
	if (!pEntity) {
		return;
	}

	const Vector3D old_position = pEntity->transform.position;
	const Vector3D position_step = pEntity->transform.velocity;
	Vector3D new_position = vector3D_add(old_position, position_step);

	// The square of the distance of the currently selected new position from the old position.
	// This variable is used to track which resolved new position is the shortest from the entity.
	// The squared length is used instead of the real length because it is only used for comparison.
	double step_length_squared = SQUARE(position_step.x) + SQUARE(position_step.y) + SQUARE(position_step.z); 

	for (unsigned int i = 0; i < currentArea.pRooms[currentArea.currentRoomIndex].num_walls; ++i) {
		
		const rect_t wall = currentArea.pRooms[currentArea.currentRoomIndex].walls[i];

		Vector3D resolved_position = resolve_collision(old_position, new_position, pEntity->hitbox, wall);
		Vector3D resolved_step = vector3D_subtract(resolved_position, old_position);
		const double resolved_step_length_squared = SQUARE(resolved_step.x) + SQUARE(resolved_step.y) + SQUARE(resolved_step.z);

		if (resolved_step_length_squared < step_length_squared) {
			new_position = resolved_position;
			step_length_squared = resolved_step_length_squared;
		}
	}

	pEntity->transform.position = new_position;

	// Update render object.
	const int renderHandle = pEntity->render_handle;
	renderObjectSetPosition(renderHandle, 0, pEntity->transform.position);
}
