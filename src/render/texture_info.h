#ifndef ANIMATION_H
#define ANIMATION_H

/* ANIMATION
 *
 * An animation struct represents a hard-coded animation cycle.
 * Data in animation_t:
 * 	uint32_t start_frame - determines the cell in the base atlas in which this cycle starts.
 * 	uint32_t num_frames - number of frames in this cycle.
 * 	
*/

#include <stdbool.h>
#include <stdint.h>

#include "util/extent.h"



// Contains information necessary to create an animation in the texture creation process.
typedef struct animation_create_info_t {

	extent_t cell_extent;
	uint32_t start_cell;
	uint32_t num_frames;
	uint32_t frames_per_second;

} animation_create_info_t;

// TODO - rename to texture_info_type_t.
typedef enum texture_type_t {
	TEXTURE_TYPE_NORMAL = 0,
	TEXTURE_TYPE_TILEMAP = 1
} texture_type_t;

// `animation_set_t` contains information to read image data from a buffer 
// 	and create arrays of images for an entire set of animations.
typedef struct texture_create_info_t {

	// The path to the image that will be loaded during image creation.
	char *path;

	texture_type_t type;

	// Dimensions of the base texture atlas, in texels.
	extent_t atlas_extent;

	// Number of cells in the base texture atlas, in each dimension.
	extent_t num_cells;

	// The dimensions of each cell, in texels.
	// TODO - delete this, eventually.
	extent_t cell_extent;

	uint32_t num_animations;
	animation_create_info_t *animations;

} texture_create_info_t;

// Contains an array of texture create infos.
// If this is dynamically created, pass it to `destroy_texture_pack` when no longer in use.
typedef struct texture_pack_t {
	uint32_t num_textures;
	texture_create_info_t *texture_create_infos;
} texture_pack_t;



bool is_animation_set_empty(texture_create_info_t texture_create_info);

// Use this to destroy a texture pack struct that has been dynamically allocated.
void destroy_texture_pack(texture_pack_t *texture_pack_ptr);

texture_pack_t parse_fgt_file(const char *filename);



#endif	// ANIMATION_H
