#include "texture_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

#include <vulkan/vulkan.h>

#include "config.h"
#include "log/logging.h"
#include "render/stb/image_data.h"
#include "util/allocate.h"

#include "buffer.h"
#include "command_buffer.h"
#include "texture.h"
#include "texture_loader.h"
#include "vulkan_manager.h"
#include "compute/compute_room_texture.h"



/* Texture manager
 *
 * Todo:
 *  - vectorize image upload to GPU
 *  - make room texture caches into image arrays instead of separate images
*/



#define TEXTURE_PATH (RESOURCE_PATH "assets/textures/")

#define NUM_RESERVED_TEXTURES 2
#define NUM_LOADED_TEXTURES 3
#define NUM_TEXTURES NUM_RESERVED_TEXTURES + NUM_LOADED_TEXTURES

static const uint32_t num_reserved_textures = NUM_RESERVED_TEXTURES;
const uint32_t num_loaded_textures = NUM_LOADED_TEXTURES;
const uint32_t num_textures = NUM_TEXTURES;

static texture_t textures[NUM_TEXTURES];

const texture_handle_t missing_texture_handle = 0;
const texture_handle_t room_texture_handle = 1;

#define TEXTURE_NAME_MAX_LENGTH 256

static const size_t texture_name_max_length = TEXTURE_NAME_MAX_LENGTH;
static char texture_names[NUM_TEXTURES][TEXTURE_NAME_MAX_LENGTH];

static const texture_create_info_t missing_texture_create_info = {
	.path = "missing.png",
	.atlas_extent.width = 16,
	.atlas_extent.length = 16,
	.num_cells.width = 1,
	.num_cells.length = 1,
	.cell_extent.width = 16,
	.cell_extent.length = 16,
	.num_animations = 1,
	.animations = (animation_create_info_t[1]){
		{
			.cell_extent = { 16, 16 },
			.start_cell = 0,
			.num_frames = 1,
			.frames_per_second = 0
		}
	}
};

texture_create_info_t make_texture_create_info(extent_t texture_extent) {

	static const animation_create_info_t animations[1] = {
		{
			.start_cell = 0,
			.num_frames = 1,
			.frames_per_second = 0
		}
	};

	texture_create_info_t texture_create_info = { 0 };
	texture_create_info.atlas_extent = texture_extent;
	texture_create_info.num_cells = (extent_t){ 1, 1 };
	texture_create_info.cell_extent = texture_extent;
	texture_create_info.num_animations = 1;
	texture_create_info.animations = (animation_create_info_t *)animations;

	return texture_create_info;
}

static void set_texture_name(const uint32_t index, const texture_create_info_t texture_create_info) {
	
	// Temporary name buffer -- do the string operations here.
	char name[TEXTURE_NAME_MAX_LENGTH];
	memset(name, '\0', texture_name_max_length);
	const errno_t texture_name_strncpy_result_1 = strncpy_s(name, texture_name_max_length, texture_create_info.path, 64);
	if (texture_name_strncpy_result_1 != 0) {
		logf_message(WARNING, "Warning loading texture: failed to copy texture path into temporary texture name buffer. (Error code: %u)", texture_name_strncpy_result_1);
	}

	// Prune off the file extension if it is found.
	// TODO - make a safer version of this function...
	char *file_extension_delimiter_ptr = strrchr(name, '.');
	if (file_extension_delimiter_ptr != NULL) {
		const ptrdiff_t file_extension_delimiter_offset = file_extension_delimiter_ptr - name;
		const size_t remaining_length = file_extension_delimiter_offset >= 0 
			? texture_name_max_length - file_extension_delimiter_offset
			: texture_name_max_length;
		memset(file_extension_delimiter_ptr, '\0', remaining_length);
	}

	// Copy the processed texture name into the array of texture names.
	const errno_t texture_name_strncpy_result_2 = strncpy_s(texture_names[index], texture_name_max_length, name, texture_name_max_length);
	if (texture_name_strncpy_result_2 != 0) {
		logf_message(WARNING, "Warning loading texture: failed to copy texture name from temporary buffer into texture name array. (Error code: %u)", texture_name_strncpy_result_2);
	}
}

void load_textures(const texture_pack_t texture_pack) {

	log_message(VERBOSE, "Loading textures...");

	if (texture_pack.num_textures == 0) {
		log_message(WARNING, "Warning loading textures: loaded texture pack is empty.");
	}

	if (texture_pack.texture_create_infos == NULL) {
		log_message(ERROR, "Error loading textures: array of texture create infos is NULL.");
		return;
	}
	
	// Nullify each texture first.
	for (uint32_t i = 0; i < num_textures; ++i) {
		textures[i] = make_null_texture();
	}

	textures[0] = load_texture(missing_texture_create_info);
	textures[1] = init_room_texture();

	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {
		// Offset for room textures and missing texture placeholder.
		const uint32_t texture_index = num_reserved_textures + i;
		textures[texture_index] = load_texture(texture_pack.texture_create_infos[i]);
		set_texture_name(i, texture_pack.texture_create_infos[i]);
	}

	log_message(VERBOSE, "Done loading textures.");
}

void create_room_texture(const room_t room, const uint32_t cache_slot, const texture_handle_t tilemap_texture_handle) {
	compute_room_texture(room, cache_slot, textures[tilemap_texture_handle], &textures[room_texture_handle]);
}

void destroy_textures(void) {

	log_message(VERBOSE, "Destroying loaded textures...");

	for (uint32_t i = 0; i < NUM_TEXTURES; ++i) {
		destroy_texture(textures + i);
	}
}

texture_t get_room_texture(const room_size_t room_size) {
	return textures[num_reserved_textures + (uint32_t)room_size];
}

texture_t get_loaded_texture(const texture_handle_t texture_handle) {
	
	if (texture_handle >= num_textures) {
		logf_message(ERROR, "Error getting loaded texture: texture handle (%u) is not less than number of loaded textures (%u).", texture_handle, num_textures);
		return textures[missing_texture_handle];
	}

	return textures[texture_handle];
}

texture_handle_t find_loaded_texture_handle(const char *restrict const texture_name) {

	if (texture_name == NULL) {
		log_message(ERROR, "Error finding loaded texture: given texture name is NULL.");
		return missing_texture_handle;
	}

	const size_t texture_name_length = strnlen_s(texture_name, texture_name_max_length);

	// Depth-first search for matching texture name.
	for (uint32_t i = 0; i < num_textures; ++i) {
	
		bool texture_name_matches = true;
		for (size_t j = 0; j < texture_name_length; ++j) {
			const char c = texture_name[j];
			const char d = texture_names[i][j];
			if (c != d) {
				texture_name_matches = false;
				break;
			}
		}

		if (texture_name_matches) {
			return (texture_handle_t)(num_reserved_textures + i);
		}
	}

	logf_message(WARNING, "Warning finding loaded texture: texture \"%s\" not found.", texture_name);
	return missing_texture_handle;
}

texture_state_t missing_texture_state(void) {

	texture_state_t texture_state = { 0 };
	texture_state.handle = missing_texture_handle;
	texture_state.num_animation_cycles = 1;
	if (!allocate((void **)&texture_state.animation_cycles, texture_state.num_animation_cycles, sizeof(texture_animation_cycle_t))) {
		log_message(ERROR, "Error making missing texture state: failed to allocate animation cycle pointer-array.");
		return (texture_state_t){ 0 };
	}
	texture_state.animation_cycles[0].num_frames = 1;
	texture_state.animation_cycles[0].frames_per_second = 0;
	texture_state.current_frame = 0;
	texture_state.current_animation_cycle = 0;
	texture_state.last_frame_time_ms = 0;

	return texture_state;
}

texture_state_t make_new_texture_state(const texture_handle_t texture_handle) {

	texture_state_t texture_state = { 0 };
	texture_state.handle = texture_handle;

	const texture_t texture = get_loaded_texture(texture_state.handle);

	texture_state.num_animation_cycles = texture.num_images;
	if (texture_state.num_animation_cycles == 0) {
		log_message(ERROR, "Error making new texture state: number of animation cycles is zero.");
		return texture_state;
	}

	if (!allocate((void **)&texture_state.animation_cycles, texture_state.num_animation_cycles, sizeof(texture_animation_cycle_t))) {
		log_message(ERROR, "Error making new texture state: failed to allocate animation cycle pointer-array.");
		return texture_state;
	}

	for (uint32_t i = 0; i < texture_state.num_animation_cycles; ++i) {
		texture_state.animation_cycles[i].num_frames = texture.images[i].image_array_length;
		texture_state.animation_cycles[i].frames_per_second = texture.images[i].frames_per_second;
	}

	texture_state.current_animation_cycle = 0;
	texture_state.current_frame = 0;
	texture_state.last_frame_time_ms = 0;

	return texture_state;
}
