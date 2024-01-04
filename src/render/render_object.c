#include "render_object.h"

#include <stddef.h>

#include "log/logging.h"
#include "util/bit.h"

const uint32_t render_handle_invalid = UINT32_MAX;
const uint32_t render_handle_dangling = render_handle_invalid - 1;

render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];

static uint64_t loaded_render_slot_flags = 3;

render_handle_t load_render_object(void) {

	if (loaded_render_slot_flags == UINT64_MAX) {
		return render_handle_invalid;
	}

	for (uint32_t i = 0; i < num_render_object_slots && i < 64; ++i) {
		if (is_bit_off(loaded_render_slot_flags, i)) {
			loaded_render_slot_flags = set_bit_on(loaded_render_slot_flags, i);
			return (render_handle_t)i;
		}
	}
	return render_handle_invalid;
}

void unload_render_object(render_handle_t *handle_ptr) {

	if (handle_ptr == NULL) {
		log_message(ERROR, "Error unloading render object: pointer to handle is NULL.");
		return;
	}

	if (!validate_render_handle(*handle_ptr)) {
		logf_message(ERROR, "Error unloading render object: handle is invalid (%u).", *handle_ptr);
		return;
	}

	if (is_bit_off(loaded_render_slot_flags, *handle_ptr)) {
		logf_message(WARNING, "Unloading already unused render object slot (%u).", *handle_ptr);
	}

	loaded_render_slot_flags = set_bit_off(loaded_render_slot_flags, *handle_ptr);
	*handle_ptr = render_handle_dangling;
}

bool validate_render_handle(render_handle_t handle) {
	return handle < num_render_object_slots && handle < flags_bitwidth;
}

bool is_render_handle_to_room_render_object(render_handle_t handle) {
	return handle < num_room_render_object_slots;
}

render_position_t *get_render_position_ptr(render_handle_t handle) {

	if (handle >= num_render_object_slots) {
		logf_message(ERROR, "Error getting render position pointer: render object handle (%u) exceeds total number of render object slots (%u).", handle, num_render_object_slots);
		return NULL;
	}

	return render_object_positions + handle;
}

void update_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {

	if (render_position_ptr == NULL) {
		return;
	}

	render_position_ptr->previous_position = render_position_ptr->position;
	render_position_ptr->position = new_position;
}

void reset_render_position(render_position_t *render_position_ptr, vector3F_t new_position) {

	if (render_position_ptr == NULL) {
		return;
	}

	render_position_ptr->position = new_position;
	render_position_ptr->previous_position = new_position;
}
