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



typedef struct animation_t {

	uint32_t start_cell;
	uint32_t num_frames;
	uint32_t frames_per_second;

} animation_t;

// `animation_set_t` contains information to read image data from a buffer 
// 	and create arrays of images for an entire set of animations.
typedef struct animation_set_t {

	// Dimensions of the base texture atlas, in texels.
	extent_t atlas_extent;

	// Number of cells in the base texture atlas, in each dimension.
	extent_t num_cells;

	// The dimensions of each cell, in texels.
	extent_t cell_extent;

	uint32_t num_animations;
	animation_t *animations;

} animation_set_t;



bool is_animation_set_empty(animation_set_t animation_set);



#endif	// ANIMATION_H
