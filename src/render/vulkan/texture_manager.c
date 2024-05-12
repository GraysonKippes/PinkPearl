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

#define TEXTURE_PATH (RESOURCE_PATH "assets/textures/")

#define NUM_RESERVED_TEXTURES 2
#define NUM_ROOM_TEXTURES 4
#define NUM_LOADED_TEXTURES 3
#define NUM_TEXTURES (NUM_RESERVED_TEXTURES + NUM_ROOM_TEXTURES + NUM_LOADED_TEXTURES)

static const uint32_t num_reserved_textures = NUM_RESERVED_TEXTURES;
static const uint32_t num_room_textures = NUM_ROOM_TEXTURES;
static const uint32_t num_loaded_textures = NUM_LOADED_TEXTURES;
const uint32_t num_textures = NUM_TEXTURES;

static texture_t textures[NUM_TEXTURES];

const texture_handle_t missing_texture_handle = 0;
const texture_handle_t room_texture_handle = 1;

// Record for a *loaded* texture.
// Textures that are generated rather than loaded (e.g. room textures) do not have records.
typedef struct texture_record_t {
	string_t texture_id;
	texture_handle_t texture_handle;
} texture_record_t;

texture_record_t texture_records[NUM_LOADED_TEXTURES];

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
			.start_cell = 0,
			.num_frames = 1,
			.frames_per_second = 0
		}
	}
};

static void register_loaded_texture(const texture_handle_t texture_handle, const texture_create_info_t texture_create_info) {
	
	string_t texture_id = new_string(256, texture_create_info.path);
	const size_t file_extension_delimiter_index = string_reverse_search_char(texture_id, '.');
	if (file_extension_delimiter_index < texture_id.length) {
		const size_t num_chars_to_remove = texture_id.length - file_extension_delimiter_index;
		string_remove_trailing_chars(&texture_id, num_chars_to_remove);
	}
	
	const texture_record_t texture_record = {
		.texture_id = texture_id,
		.texture_handle = texture_handle
	};
	
	size_t hash_index = string_hash(texture_id, (size_t)num_loaded_textures);
	for (size_t i = 0; i < (size_t)num_loaded_textures; ++i) {
		if (is_string_null(texture_records[hash_index].texture_id)) {
			texture_records[hash_index] = texture_record;
		}
		else {
			hash_index += 1;
			hash_index %= (size_t)num_loaded_textures;
		}
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
	textures[1] = init_room_texture(ONE_TO_THREE);
	textures[2] = init_room_texture(TWO_TO_THREE);
	textures[3] = init_room_texture(THREE_TO_THREE);
	textures[4] = init_room_texture(FOUR_TO_THREE);

	// Nullify texture records FIRST, hash-collision-resolution relies on records being null to begin with.
	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {
		texture_records[i].texture_id = make_null_string();
		texture_records[i].texture_handle = missing_texture_handle;
	}

	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {
		
		// Offset for room textures and missing texture placeholder.
		const texture_handle_t texture_handle = num_reserved_textures + num_room_textures + i;
		const texture_create_info_t texture_create_info = texture_pack.texture_create_infos[i];
		
		textures[texture_handle] = load_texture(texture_create_info);
		register_loaded_texture(texture_handle, texture_create_info);
	}

	log_message(VERBOSE, "Done loading textures.");
}

void create_room_texture(const room_t room, const uint32_t cache_slot, const texture_handle_t tilemap_texture_handle) {
	compute_room_texture(room, cache_slot, textures[tilemap_texture_handle], &textures[room_texture_handle + (uint32_t)room.size]);
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

texture_handle_t find_loaded_texture_handle(const string_t texture_id) {
	if (is_string_null(texture_id)) {
		log_message(ERROR, "Error finding loaded texture: given texture ID is NULL.");
		return missing_texture_handle;
	}
	
	size_t hash_index = string_hash(texture_id, (size_t)num_loaded_textures);
	for (size_t i = 0; i < (size_t)num_loaded_textures; ++i) {
		if (string_compare(texture_id, texture_records[i].texture_id)) {
			return texture_records[i].texture_handle;
		}
		else {
			hash_index++;
			hash_index %= num_loaded_textures;
		}
	}
	return missing_texture_handle;
}

texture_state_t missing_texture_state(void) {

	texture_state_t texture_state = { 0 };
	texture_state.handle = missing_texture_handle;
	texture_state.num_animations = 1;
	texture_state.current_frame = 0;
	texture_state.current_animation = 0;
	texture_state.last_frame_time_ms = 0;

	return texture_state;
}

texture_state_t make_new_texture_state(const texture_handle_t texture_handle) {
	const texture_t texture = get_loaded_texture(texture_handle);
	return (texture_state_t){
		.handle = texture_handle,
		.num_animations = texture.num_animations,
		.current_animation = 0,
		.current_frame = 0,
		.last_frame_time_ms = 0
	};
}
