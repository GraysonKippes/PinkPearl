#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "game/area/room.h"

#include "../texture.h"
#include "../TextureState.h"

void init_compute_room_texture(const VkDevice vkDevice);

void terminate_compute_room_texture(void);

void computeStitchTexture(const int tilemapTextureHandle, const int destinationTextureHandle, const ImageSubresourceRange destinationRange, const Extent tileExtent, uint32_t **tileIndices);

#endif	// COMPUTE_ROOM_TEXTURE_H
