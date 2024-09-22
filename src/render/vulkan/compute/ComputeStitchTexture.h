#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "game/area/room.h"

#include "../texture.h"
#include "../TextureState.h"

void initComputeStitchTexture(const VkDevice vkDevice);

void terminateComputeStitchTexture(void);

void computeStitchTexture(const int tilemapTextureHandle, const int destinationTextureHandle, const ImageSubresourceRange destinationRange, const Extent tileExtent, uint32_t **tileIndices);

#endif	// COMPUTE_ROOM_TEXTURE_H
