#include "texture_state.h"

#include <stddef.h>

#include "log/logging.h"
#include "util/allocate.h"
#include "util/time.h"
#include "vulkan/texture.h"
#include "vulkan/texture_manager.h"

bool destroy_texture_state(texture_state_t *texture_state_ptr) {
	if (texture_state_ptr == NULL) {
		return false;
	}

	texture_state_ptr->handle = 0;
	texture_state_ptr->num_animations = 0;
	texture_state_ptr->current_animation = 0;
	texture_state_ptr->current_frame = 0;
	texture_state_ptr->last_frame_time_ms = 0;

	return true;
}

bool texture_state_set_animation_cycle(texture_state_t *const texture_state_ptr, const uint32_t next_animation) {
	if (texture_state_ptr == NULL) {
		return false;
	}

	if (next_animation >= texture_state_ptr->num_animations) {
		return false;
	}

	texture_state_ptr->current_animation = next_animation;
	texture_state_ptr->current_frame = 0;
	texture_state_ptr->last_frame_time_ms = get_time_ms();

	return true;
}

int texture_state_animate(texture_state_t *const texture_state_ptr) {
	if (texture_state_ptr == NULL) {
		return 0;
	}

	if (texture_state_ptr->current_animation >= texture_state_ptr->num_animations) {
		logf_message(WARNING, "Warning updating texture animation state: current animation index (%u) is not less than number of animations (%u).", texture_state_ptr->current_animation, texture_state_ptr->num_animations);
		texture_state_ptr->current_animation = 0;
		return 0;
	}

	if (texture_state_ptr->last_frame_time_ms == 0) {
		texture_state_ptr->last_frame_time_ms = get_time_ms();
		return 1;
	}
	
	const texture_t texture = get_loaded_texture(texture_state_ptr->handle);
	const texture_animation_t animation = texture.animations[texture_state_ptr->current_animation];
	const uint64_t current_time_ms = get_time_ms();

	// Calculate the time difference between last frame change for this texture and current time, in seconds.
	const uint64_t delta_time_ms = current_time_ms - texture_state_ptr->last_frame_time_ms;
	const double delta_time_s = (double)delta_time_ms / 1000.0;

	// Time in seconds * frames per second = number of frames to increment the frame counter by.
	const uint32_t frames = (uint32_t)(delta_time_s * (double)animation.frames_per_second);

	// Update the current frame and last frame time of this texture.
	if (frames > 0) {
		texture_state_ptr->current_frame += frames;
		texture_state_ptr->current_frame %= animation.num_frames;
		texture_state_ptr->last_frame_time_ms = current_time_ms;
		return 2;
	}

	return 1;
}
