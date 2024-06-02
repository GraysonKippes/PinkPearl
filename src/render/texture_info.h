#ifndef TEXTURE_INFO_H
#define TEXTURE_INFO_H

#include <stdbool.h>
#include <stdint.h>

#include "math/extent.h"
#include "util/string.h"

// Contains information necessary to create an animation in the texture creation process.
// TODO - replace with TextureAnimation.
typedef struct animation_create_info_t {
	uint32_t start_cell;
	uint32_t num_frames;
	uint32_t frames_per_second;
} animation_create_info_t;

// `animation_set_t` contains information to read image data from a buffer 
// 	and create arrays of images for an entire set of animations.
typedef struct TextureCreateInfo {
	
	String textureID;
	
	bool isLoaded;
	bool isTilemap;

	// Dimensions of the base texture atlas, in texels.
	// TODO - remove this.
	extent_t atlas_extent;

	// Number of cells in the base texture atlas, in each dimension.
	extent_t num_cells;

	// The dimensions of each cell, in texels.
	extent_t cell_extent;

	uint32_t num_animations;
	animation_create_info_t *animations;

} TextureCreateInfo;

// Contains an array of texture create infos.
// If this is dynamically created, pass it to `destroy_texture_pack` when no longer in use.
typedef struct TexturePack {
	uint32_t num_textures;
	TextureCreateInfo *texture_create_infos;
} TexturePack;

// TODO - add validateTextureCreateInfo

// Use this to destroy a texture pack struct that has been dynamically allocated.
void destroy_texture_pack(TexturePack *const pTexturePack);

TexturePack parse_fgt_file(const char *filename);

#endif	// TEXTURE_INFO_H
