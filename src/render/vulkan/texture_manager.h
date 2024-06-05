#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "render/texture_pack.h"
#include "util/string.h"

#include "texture.h"

extern const int numTextures;
extern const int textureHandleMissing;

void initTextureManager(void);
void terminateTextureManager(void);

// Loads all the textures inside texturePack into the texture manager.
bool textureManagerLoadTexturePack(const TexturePack texturePack);

// Creates a texture and loads it into the texture manager.
void textureManagerLoadTexture(const TextureCreateInfo textureCreateInfo);

// Returns true if the texture handle is a valid texture handle, false otherwise.
bool validateTextureHandle(const int textureHandle);

// Finds a texture with the given texture ID and returns a handle to it.
int findTexture(const String textureID);

// Returns a texture from the array of loaded texture directly from the texture handle.
Texture getTexture(const int textureHandle);

// Returns the pointer to a texture from the array of loaded texture directly from the texture handle.
Texture *getTextureP(const int textureHandle);

#endif	// TEXTURE_MANAGER_H
