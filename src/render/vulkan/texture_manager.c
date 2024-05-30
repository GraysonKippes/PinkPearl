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

#define NUM_RESERVED_TEXTURES 1
#define NUM_ROOM_TEXTURES 4
#define NUM_LOADED_TEXTURES 3
#define NUM_TEXTURES (NUM_RESERVED_TEXTURES + NUM_ROOM_TEXTURES + NUM_LOADED_TEXTURES)

static const int numReservedTextures = NUM_RESERVED_TEXTURES;
static const int numRoomTextures = NUM_ROOM_TEXTURES;
static const int numLoadedTextures = NUM_LOADED_TEXTURES;
const int numTextures = NUM_TEXTURES;

static Texture textures[NUM_TEXTURES];

const int missing_texture_handle = 0;

// Record for a *loaded* texture.
// Textures that are generated rather than loaded (e.g. room textures) do not have records.
typedef struct TextureRecord {
	String texture_id;
	int texture_handle;
} TextureRecord;

TextureRecord texture_records[NUM_LOADED_TEXTURES];

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

static void register_loaded_texture(const int texture_handle, const texture_create_info_t texture_create_info) {
	
	String texture_id = newString(256, texture_create_info.path);
	const size_t file_extension_delimiter_index = stringReverseSearchChar(texture_id, '.');
	if (file_extension_delimiter_index < texture_id.length) {
		const size_t num_chars_to_remove = texture_id.length - file_extension_delimiter_index;
		stringRemoveTrailingChars(&texture_id, num_chars_to_remove);
	}
	
	const TextureRecord texture_record = {
		.texture_id = texture_id,
		.texture_handle = texture_handle
	};
	
	size_t hash_index = stringHash(texture_id, (size_t)numLoadedTextures);
	for (size_t i = 0; i < (size_t)numLoadedTextures; ++i) {
		if (stringIsNull(texture_records[hash_index].texture_id)) {
			texture_records[hash_index] = texture_record;
		}
		else {
			hash_index += 1;
			hash_index %= (size_t)numLoadedTextures;
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
	for (int i = 0; i < numTextures; ++i) {
		textures[i] = make_null_texture();
	}

	textures[0] = loadTexture(missing_texture_create_info);
	textures[1] = createRoomTexture(ONE_TO_THREE);
	textures[2] = createRoomTexture(TWO_TO_THREE);
	textures[3] = createRoomTexture(THREE_TO_THREE);
	textures[4] = createRoomTexture(FOUR_TO_THREE);

	// Nullify texture records FIRST, hash-collision-resolution relies on records being null to begin with.
	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {
		texture_records[i].texture_id = makeNullString();
		texture_records[i].texture_handle = missing_texture_handle;
	}

	for (uint32_t i = 0; i < texture_pack.num_textures; ++i) {
		
		// Offset for room textures and missing texture placeholder.
		const int texture_handle = numReservedTextures + numRoomTextures + i;
		const texture_create_info_t texture_create_info = texture_pack.texture_create_infos[i];
		
		textures[texture_handle] = loadTexture(texture_create_info);
		register_loaded_texture(texture_handle, texture_create_info);
	}

	log_message(VERBOSE, "Done loading textures.");
}

// TODO - remove
void create_room_texture(const room_t room, const uint32_t cacheSlot, const int tilemapTextureHandle) {
	const int roomTextureHandle = numReservedTextures + (int)room.size;
	compute_room_texture(room, cacheSlot, textures[tilemapTextureHandle], &textures[roomTextureHandle]);
}

void destroy_textures(void) {
	log_message(VERBOSE, "Destroying loaded textures...");
	for (uint32_t i = 0; i < NUM_TEXTURES; ++i) {
		destroy_texture(textures + i);
	}
}

// TODO - remove
Texture get_room_texture(const room_size_t room_size) {
	return textures[numReservedTextures + (uint32_t)room_size];
}

bool validateTextureHandle(const int textureHandle) {
	return textureHandle >= 0 && textureHandle < numTextures;
}

int findTexture(const String textureID) {
	if (stringIsNull(textureID)) {
		log_message(ERROR, "Error finding loaded texture: given texture ID is NULL.");
		return missing_texture_handle;
	}
	
	size_t hash_index = stringHash(textureID, (size_t)numLoadedTextures);
	for (size_t i = 0; i < (size_t)numLoadedTextures; ++i) {
		if (stringCompare(textureID, texture_records[i].texture_id)) {
			return texture_records[i].texture_handle;
		}
		else {
			hash_index++;
			hash_index %= numLoadedTextures;
		}
	}
	return missing_texture_handle;
}

Texture getTexture(const int textureHandle) {
	if (!validateTextureHandle(textureHandle)) {
		//logf_message(ERROR, "Error getting loaded texture: texture handle (%u) is invalid.", textureHandle);
		return textures[missing_texture_handle];
	}
	return textures[textureHandle];
}
