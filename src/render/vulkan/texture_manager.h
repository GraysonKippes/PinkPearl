#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "game/area/room.h"
#include "render/texture_info.h"
#include "util/string.h"

#include "texture.h"

extern const int num_textures;
extern const int missing_texture_handle;

void load_textures(const texture_pack_t texture_pack);
void create_room_texture(const room_t room, const uint32_t cacheSlot, const int tilemapTextureHandle);	// TODO - remove
void destroy_textures(void);

// Returns true if the texture handle is a valid texture handle, false otherwise.
bool validateTextureHandle(const int textureHandle);

// Finds a texture with the given texture ID and returns a handle to it.
int findTexture(const string_t textureID);

// Gets a texture from the array of loaded texture directly from the texture handle.
Texture getTexture(const int textureHandle);

#endif	// TEXTURE_MANAGER_H
