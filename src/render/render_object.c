#include "render_object.h"

#include <stddef.h>

#include "log/logging.h"
#include "util/bit.h"

const uint32_t render_handle_invalid = UINT32_MAX;
const uint32_t render_handle_dangling = render_handle_invalid - 1;

render_position_t render_object_positions[NUM_RENDER_OBJECT_SLOTS];
texture_state_t render_object_texture_states[NUM_RENDER_OBJECT_SLOTS];

static uint64_t render_object_slot_enabled_flags = 0;

render_handle_t load_render_object(void) {

	if (render_object_slot_enabled_flags == UINT64_MAX) {
		return render_handle_invalid;
	}

	for (uint32_t i = 0; i < num_render_object_slots && i < 64; ++i) {
		if (is_bit_off(render_object_slot_enabled_flags, i)) {
			enable_render_object_slot(i);
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

	if (is_bit_off(render_object_slot_enabled_flags, *handle_ptr)) {
		logf_message(WARNING, "Unloading already unused render object slot (%u).", *handle_ptr);
	}

	render_object_slot_enabled_flags = set_bit_off(render_object_slot_enabled_flags, *handle_ptr);
	*handle_ptr = render_handle_dangling;
}

bool is_render_object_slot_enabled(const uint32_t slot) {
	if (validate_render_handle(slot)) {
		return (render_object_slot_enabled_flags >> slot) & 1LL;
	}
	return false;
}

void enable_render_object_slot(const uint32_t slot) {
	if (validate_render_handle(slot)) {
		render_object_slot_enabled_flags |= (1LL << (uint64_t)slot);
	}
}

bool validate_render_handle(render_handle_t handle) {
	return handle < num_render_object_slots && handle < flags_bitwidth;
}

render_position_t *get_render_position_ptr(render_handle_t handle) {

	if (handle >= num_render_object_slots) {
		logf_message(ERROR, "Error getting render position pointer: render object handle (%u) exceeds total number of render object slots (%u).", handle, num_render_object_slots);
		return NULL;
	}

	return render_object_positions + handle;
}

bool swap_render_object_texture_state(const render_handle_t render_handle, const texture_state_t texture_state) {

	if (!validate_render_handle(render_handle)) {
		return false;
	}

	destroy_texture_state(&render_object_texture_states[render_handle]);
	render_object_texture_states[render_handle] = texture_state;
	return true;
}
