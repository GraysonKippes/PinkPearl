#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include <stdint.h>

#define NUM_FRAMES_IN_FLIGHT 2
#define NUM_RENDER_OBJECT_SLOTS 64
#define NUM_ROOM_TEXTURE_CACHE_SLOTS 2

extern const uint32_t num_frames_in_flight;
extern const uint32_t num_render_object_slots;

// This config variable controls how many images for the room texture are loaded at a time.
// Because multiple rooms are visible when scrolling between them, at least two images
// must be available for rendering. Therefore, this variable must be at least two.
extern const uint32_t num_room_texture_cache_slots;

#define VERTEX_SHADER_NAME 				"vertex_shader.spv"
#define FRAGMENT_SHADER_NAME 			"fragment_shader.spv"
#define COMPUTE_MATRICES_SHADER_NAME 	"compute_matrices.spv"
#define AREA_MESH_SHADER_NAME			"area_mesh.spv"
#define ROOM_TEXTURE_SHADER_NAME 		"room_texture.spv"

#endif	// RENDER_CONFIG_H
