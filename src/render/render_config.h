#ifndef RENDER_CONFIG_H
#define RENDER_CONFIG_H

#include <stdint.h>

#define NUM_FRAMES_IN_FLIGHT 2

#define NUM_RENDER_OBJECT_SLOTS 32

#define MAX_NUM_RENDER_OBJECT_QUADS 8

#define NUM_ROOM_TEXTURE_CACHE_SLOTS 2

#define NUM_ROOM_LAYERS 2

#define TILE_TEXEL_LENGTH 16

extern const uint32_t num_frames_in_flight;

// The total number of render object slots available.
extern const uint32_t numRenderObjectSlots;

extern const int maxNumRenderObjectQuads;

// This config variable controls how many images for the room texture are loaded at a time.
// Because multiple rooms are visible when scrolling between them, at least two images
// must be available for rendering. Therefore, this variable must be at least two.
extern const uint32_t numRoomTextureCacheSlots;

// This config variable controls how many layers there are in each room.
// Currently, there are two layers:
// 	0. Background,
// 	1. Foreground.
extern const uint32_t numRoomLayers;

// This config variable represents how long each tile's texture is on each side, in texels.
// By default, this is 16, for a 16x16 texture.
extern const uint32_t tile_texel_length;

#define VERTEX_SHADER_NAME 				"VertexShader.spv"
#define FRAGMENT_SHADER_NAME 			"FragmentShader.spv"
#define COMPUTE_MATRICES_SHADER_NAME 	"compute_matrices.spv"
#define AREA_MESH_SHADER_NAME			"area_mesh.spv"
#define ROOM_TEXTURE_SHADER_NAME 		"room_texture.spv"

#endif	// RENDER_CONFIG_H
