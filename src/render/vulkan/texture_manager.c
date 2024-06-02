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

// Record for a *loaded* texture.
// Textures that are generated rather than loaded (e.g. room textures) do not have records.
typedef struct TextureRecord {
	String textureID;
	int textureHandle;
} TextureRecord;

static const int numReservedTextures = NUM_RESERVED_TEXTURES;
static const int numRoomTextures = NUM_ROOM_TEXTURES;
static const int numLoadedTextures = NUM_LOADED_TEXTURES;
const int numTextures = NUM_TEXTURES;

static Texture textures[NUM_TEXTURES];
static TextureRecord textureRecords[NUM_TEXTURES];

// Locks texture loading once all textures are loaded.
static bool texturesLoaded = false;

const int textureHandleMissing = 0;

//TextureCreateInfo missingTextureCreateInfo = { 0 };

// Creates a texture record and hashes it.
static void registerTexture(const int textureHandle, const TextureCreateInfo textureCreateInfo) {
	
	const TextureRecord textureRecord = {
		.textureID = deepCopyString(textureCreateInfo.textureID),
		.textureHandle = textureHandle
	};
	
	size_t hashIndex = stringHash(textureRecord.textureID, (size_t)numTextures);
	for (size_t i = 0; i < (size_t)numTextures; ++i) {
		if (stringIsNull(textureRecords[hashIndex].textureID)) {
			textureRecords[hashIndex] = textureRecord;
			break;
		} else {
			hashIndex += 1;
			hashIndex %= (size_t)numLoadedTextures;
		}
	}
}

static void initTexture(const int textureHandle, const TextureCreateInfo textureCreateInfo) {
	logf_message(VERBOSE, "Initializing texture with handle %i, ID \"%s\"...", textureHandle, textureCreateInfo.textureID.buffer);
	
	if (!validateTextureHandle(textureHandle)) {
		logf_message(ERROR, "Error initializing texture: invalid texture handle (%i).", textureHandle);
		return;
	}
	
	if (!textureIsNull(textures[textureHandle])) {
		logf_message(ERROR, "Error initializing texture: texture %i already initialized.", textureHandle);
		return;
	}
	
	Texture texture = makeNullTexture();
	if (textureCreateInfo.isLoaded) {
		texture = loadTexture(textureCreateInfo);
	} else {
		texture = createTexture(textureCreateInfo);
	}
	
	textures[textureHandle] = texture;
	registerTexture(textureHandle, textureCreateInfo);
}

void initTextureManager(const TexturePack texturePack) {
	log_message(VERBOSE, "Loading textures...");
	
	if (texturesLoaded) {
		log_message(WARNING, "Warning loading textures: textures already loaded.");
		return;
	}

	if (texturePack.num_textures == 0) {
		log_message(WARNING, "Warning loading textures: loaded texture pack is empty.");
	}

	if (texturePack.texture_create_infos == NULL) {
		log_message(ERROR, "Error loading textures: array of texture create infos is NULL.");
		return;
	}
	
	const TextureCreateInfo missingTextureCreateInfo = (TextureCreateInfo){
		.textureID = newString(256, "missing"),
		.isLoaded = true,
		.isTilemap = false,
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

	// Nullify each texture first.
	for (int i = 0; i < numTextures; ++i) {
		textures[i] = makeNullTexture();
	}
	
	// Nullify texture records FIRST, hash-collision-resolution relies on records being null to begin with.
	for (uint32_t i = 0; i < texturePack.num_textures; ++i) {
		textureRecords[i].textureID = makeNullString();
		textureRecords[i].textureHandle = textureHandleMissing;
	}

	textures[1] = createRoomTexture(ONE_TO_THREE);
	textures[2] = createRoomTexture(TWO_TO_THREE);
	textures[3] = createRoomTexture(THREE_TO_THREE);
	textures[4] = createRoomTexture(FOUR_TO_THREE);
	
	initTexture(0, missingTextureCreateInfo);

	for (uint32_t i = 0; i < texturePack.num_textures; ++i) {
		// Offset for room textures and missing texture placeholder.
		const int textureHandle = numReservedTextures + numRoomTextures + i;
		const TextureCreateInfo textureCreateInfo = texturePack.texture_create_infos[i];
		initTexture(textureHandle, textureCreateInfo);
	}

	texturesLoaded = true;
	log_message(VERBOSE, "Done loading textures.");
}

void terminateTextureManager(void) {
	log_message(VERBOSE, "Destroying loaded textures...");
	for (uint32_t i = 0; i < NUM_TEXTURES; ++i) {
		destroy_texture(textures + i);
	}
	texturesLoaded = false;
}

// TODO - remove
void create_room_texture(const room_t room, const uint32_t cacheSlot, const int tilemapTextureHandle) {
	const int roomTextureHandle = numReservedTextures + (int)room.size;
	compute_room_texture(room, cacheSlot, textures[tilemapTextureHandle], &textures[roomTextureHandle]);
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
		log_message(ERROR, "Error finding loaded texture: given texture ID is null.");
		return textureHandleMissing;
	}
	
	size_t hashIndex = stringHash(textureID, (size_t)numTextures);
	for (size_t i = 0; i < (size_t)numTextures; ++i) {
		if (stringCompare(textureID, textureRecords[hashIndex].textureID)) {
			return textureRecords[hashIndex].textureHandle;
		} else {
			hashIndex += 1;
			hashIndex %= numLoadedTextures;
		}
	}
	return textureHandleMissing;
}

Texture getTexture(const int textureHandle) {
	if (!validateTextureHandle(textureHandle)) {
		//logf_message(ERROR, "Error getting loaded texture: texture handle (%u) is invalid.", textureHandle);
		return textures[textureHandleMissing];
	}
	return textures[textureHandle];
}
