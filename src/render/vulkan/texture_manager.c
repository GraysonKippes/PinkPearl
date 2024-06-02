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
#define NUM_TEXTURES 67

typedef struct TextureRecord {
	String textureID;
	int textureHandle;
} TextureRecord;

static const int numReservedTextures = NUM_RESERVED_TEXTURES;

const int numTextures = NUM_TEXTURES;
int numTexturesLoaded = 0;

static Texture textures[NUM_TEXTURES];
static TextureRecord textureRecords[NUM_TEXTURES];

static bool textureManagerInitialized = false;

const int textureHandleMissing = 0;

// Creates a texture record and hashes it.
static void registerTexture(const int textureHandle, const TextureCreateInfo textureCreateInfo);

void initTextureManager(void) {
	log_message(VERBOSE, "Initializing texture manager...");
	
	if (textureManagerInitialized) {
		log_message(ERROR, "Error initializing texture manager: texture manager already initialized.");
		return;
	}

	// Nullify each texture first.
	for (int i = 0; i < numTextures; ++i) {
		textures[i] = makeNullTexture();
	}
	
	// Nullify texture records FIRST, hash-collision-resolution relies on records being null to begin with.
	for (int i = 0; i < numTextures; ++i) {
		textureRecords[i].textureID = makeNullString();
		textureRecords[i].textureHandle = textureHandleMissing;
	}
	
	TextureCreateInfo missingTextureCreateInfo = (TextureCreateInfo){
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
	initTexture(missingTextureCreateInfo);
	deleteString(&missingTextureCreateInfo.textureID);

	textureManagerInitialized = true;
	log_message(VERBOSE, "Done loading textures.");
}

void terminateTextureManager(void) {
	log_message(VERBOSE, "Terminating texture manager....");
	for (int i = 0; i < numTextures; ++i) {
		destroy_texture(&textures[i]);
	}
	textureManagerInitialized = false;
	log_message(VERBOSE, "Done terminating texture manager.");
}

bool loadTexturePack(const TexturePack texturePack) {
	log_message(VERBOSE, "Loading texture pack...");
	
	if (!textureManagerInitialized) {
		initTextureManager();
	}
	
	if (texturePack.num_textures == 0) {
		log_message(WARNING, "Warning loading texture pack: loaded texture pack is empty.");
		return false;
	}

	if (texturePack.texture_create_infos == NULL) {
		log_message(ERROR, "Error loading texture pack: array of texture create infos is null.");
		return false;
	}
	
	const int textureSlotsRemaining = numTextures - numTexturesLoaded;
	if (textureSlotsRemaining < (int)texturePack.num_textures) {
		logf_message(ERROR, "Error loading texture pack: not enough available texture slots (%i) for texture pack.", textureSlotsRemaining);
		return false;
	}
	
	for (unsigned int i = 0; i < texturePack.num_textures; ++i) {
		initTexture(texturePack.texture_create_infos[i]);
	}
	
	log_message(VERBOSE, "Done loading texture pack.");
	return true;
}

void initTexture(const TextureCreateInfo textureCreateInfo) {
	logf_message(VERBOSE, "Initializing texture \"%s\"...", textureCreateInfo.textureID.buffer);
	
	const int textureHandle = numTexturesLoaded++;
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
	
	logf_message(VERBOSE, "Done initializing texture \"%s\".", textureCreateInfo.textureID.buffer);
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
			hashIndex %= (size_t)numTextures;
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
			hashIndex %= (size_t)numTextures;
		}
	}
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
