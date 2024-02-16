#ifndef TEXTURE_STATE_H
#define TEXTURE_STATE_H

#include <stdbool.h>
#include <stdint.h>

typedef uint32_t texture_handle_t;

typedef struct texture_animation_cycle_t {
	uint32_t num_frames;
	uint32_t frames_per_second;
} texture_animation_cycle_t;

typedef struct texture_state_t {

	texture_handle_t handle;

	uint32_t num_animation_cycles; // Guaranteed to be at least 1.
	texture_animation_cycle_t *animation_cycles;

	uint32_t current_animation_cycle;
	uint32_t current_frame;
	uint64_t last_frame_time_ms;

} texture_state_t;

bool destroy_texture_state(texture_state_t *texture_state_ptr);

bool texture_state_set_animation_cycle(texture_state_t *const texture_state_ptr, const uint32_t next_animation_cycle);

bool texture_state_animate(texture_state_t *const texture_state_ptr);

#endif // TEXTURE_STATE_H
