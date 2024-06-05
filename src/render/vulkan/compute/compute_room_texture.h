#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "game/area/room.h"

#include "../texture.h"

void init_compute_room_texture(const VkDevice vkDevice);

void terminate_compute_room_texture(void);

void computeRoomTexture(const room_t room, const uint32_t cacheSlot, const int tilemapTextureHandle, const int roomTextureHandle);

#endif	// COMPUTE_ROOM_TEXTURE_H
