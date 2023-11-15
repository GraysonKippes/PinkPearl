#include "render_object.h"

static render_position_t *render_positions;

render_position_t *get_render_position_ptr(render_handle_t handle) {
	return render_positions + handle;
}

void update_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {
	render_position_ptr->previous_position = render_position_ptr->position;
	render_position_ptr->position = new_position;
}

