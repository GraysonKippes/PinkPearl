#include "texture_manager.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#define __STDC_WANT_LIB_EXT1__ 1
#include <string.h>

#include <vulkan/vulkan.h>

#include "config.h"
#include "log/Logger.h"
#include "render/stb/image_data.h"
#include "util/allocate.h"

#include "buffer.h"
#include "CommandBuffer.h"
#include "texture.h"
#include "texture_loader.h"
#include "VulkanManager.h"
#include "compute/ComputeStitchTexture.h"

#define TEXTURE_PATH (RESOURCE_PATH "assets/textures/")

#define NUM_TEXTURES 67

typedef struct TextureRecord {
	String textureID;
	int textureHandle;
} TextureRecord;

const int numTextures = NUM_TEXTURES;
int numTexturesLoaded = 0;

static Texture textures[NUM_TEXTURES];
static TextureRecord textureRecords[NUM_TEXTURES];

static bool textureManagerInitialized = false;

const int textureHandleMissing = 0;

// Creates a texture record and hashes it.
static void registerTexture(const int textureHandle, const TextureCreateInfo textureCreateInfo);

void initTextureManager(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Initializing texture manager...");
	
	if (textureManagerInitialized) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing texture manager: texture manager already initialized.");
		return;
	}

	// Nullify each texture first.
	for (int i = 0; i < numTextures; ++i) {
		textures[i] = nullTexture;
	}
	
	// Nullify texture records FIRST, hash-collision-resolution relies on records being null to begin with.
	for (int i = 0; i < numTextures; ++i) {
		textureRecords[i].textureID = makeNullString();
		textureRecords[i].textureHandle = textureHandleMissing;
	}
	
	TextureCreateInfo missingTextureCreateInfo = (TextureCreateInfo){
		.textureID = (String){
			.length = 7,
			.capacity = 8,
			.pBuffer = "missing"
		},
		.isLoaded = true,
		.isTilemap = false,
		.numCells.width = 1,
		.numCells.length = 1,
		.cellExtent.width = 16,
		.cellExtent.length = 16,
		.numAnimations = 1,
		.animations = (TextureAnimation[1]){
			{
				.startCell = 0,
				.numFrames = 1,
				.framesPerSecond = 0
			}
		}
	};
	textureManagerLoadTexture(missingTextureCreateInfo);

	textureManagerInitialized = true;
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done loading textures.");
}

void terminateTextureManager(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Terminating texture manager....");
	for (int i = 0; i < numTextures; ++i) {
		if (!textureIsNull(textures[i])) {
			deleteTexture(&textures[i]);
		}
	}
	textureManagerInitialized = false;
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Terminated texture manager.");
}

bool textureManagerLoadTexturePack(const TexturePack texturePack) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Loading texture pack...");
	
	if (!textureManagerInitialized) {
		initTextureManager();
	}
	
	if (texturePack.numTextures == 0) {
		logMsg(loggerVulkan, LOG_LEVEL_WARNING, "Warning loading texture pack: loaded texture pack is empty.");
		return false;
	}

	if (texturePack.pTextureCreateInfos == nullptr) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture pack: array of texture create infos is null.");
		return false;
	}
	
	const int textureSlotsRemaining = numTextures - numTexturesLoaded;
	if (textureSlotsRemaining < (int)texturePack.numTextures) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error loading texture pack: not enough available texture slots (%i) for texture pack.", textureSlotsRemaining);
		return false;
	}
	
	for (unsigned int i = 0; i < texturePack.numTextures; ++i) {
		textureManagerLoadTexture(texturePack.pTextureCreateInfos[i]);
	}
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done loading texture pack.");
	return true;
}

void textureManagerLoadTexture(const TextureCreateInfo textureCreateInfo) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Initializing texture \"%s\"...", textureCreateInfo.textureID.pBuffer);
	
	const int textureHandle = numTexturesLoaded++;
	if (!validateTextureHandle(textureHandle)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing texture: invalid texture handle (%i).", textureHandle);
		return;
	}
	
	if (!textureIsNull(textures[textureHandle])) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error initializing texture: texture %i already initialized.", textureHandle);
		return;
	}
	
	Texture texture = nullTexture;
	if (textureCreateInfo.isLoaded) {
		texture = loadTexture(textureCreateInfo);
	} else {
		texture = createTexture(textureCreateInfo);
	}
	
	textures[textureHandle] = texture;
	registerTexture(textureHandle, textureCreateInfo);
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done initializing texture \"%s\".", textureCreateInfo.textureID.pBuffer);
}

bool validateTextureHandle(const int textureHandle) {
	return textureHandle >= 0 && textureHandle < numTextures;
}

int findTexture(const String textureID) {
	if (stringIsNull(textureID)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error finding loaded texture: given texture ID is null.");
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
	
	logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error finding loaded texture: could not find texture \"%s\".", textureID.pBuffer);
	return textureHandleMissing;
}

Texture getTexture(const int textureHandle) {
	if (!validateTextureHandle(textureHandle)) {
		//logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error getting loaded texture: texture handle (%u) is invalid.", textureHandle);
		return textures[textureHandleMissing];
	}
	return textures[textureHandle];
}

Texture *getTextureP(const int textureHandle) {
	if (!validateTextureHandle(textureHandle)) {
		return &textures[textureHandleMissing];
	}
	return &textures[textureHandle];
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
