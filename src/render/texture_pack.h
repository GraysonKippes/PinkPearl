#ifndef TEXTURE_INFO_H
#define TEXTURE_INFO_H

#include <stdbool.h>
#include <stdint.h>

#include "vulkan/texture.h"

// Contains an array of texture create infos.
// If this is dynamically created, pass it to `deleteTexturePack` when no longer in use.
typedef struct TexturePack {
	unsigned int numTextures;
	TextureCreateInfo *pTextureCreateInfos;
} TexturePack;

// Use this to destroy a texture pack struct that has been dynamically allocated.
void deleteTexturePack(TexturePack *const pTexturePack);

TexturePack readTexturePackFile(const char *pPath);

#endif	// TEXTURE_INFO_H
