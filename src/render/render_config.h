#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include <stdint.h>

#define NUM_FRAMES_IN_FLIGHT 2

extern const uint32_t num_frames_in_flight;

#define NUM_RENDER_OBJECT_SLOTS 64

extern const uint32_t num_render_object_slots;

#define NUM_ROOM_RENDER_OBJECT_SLOTS 2

extern const uint32_t num_room_render_object_slots;

#define VERTEX_SHADER_NAME 		"vertex_shader.spv"
#define FRAGMENT_SHADER_NAME 		"fragment_shader.spv"
#define COMPUTE_MATRICES_SHADER_NAME 	"compute_matrices.spv"
#define ROOM_TEXTURE_SHADER_NAME 	"room_texture.spv"

#endif	// RENDER_CONFIG_H
