#include "render_object.h"

#include <stddef.h>

#include "log/logging.h"

render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];

render_position_t *get_render_position_ptr(render_handle_t handle) {

	if (handle >= num_render_object_slots) {
		logf_message(ERROR, "Error getting render position pointer: render object handle (%u) exceeds total number of render object slots (%u).", handle, num_render_object_slots);
		return NULL;
	}

	return render_object_positions + handle;
}

void update_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {
	render_position_ptr->previous_position = render_position_ptr->position;
	render_position_ptr->position = new_position;
}

void reset_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {
	render_position_ptr->position = new_position;
	render_position_ptr->previous_position = new_position;
}
