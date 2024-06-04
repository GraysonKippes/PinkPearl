#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "game/area/room.h"

#include "../texture.h"

void init_compute_room_texture(const VkDevice vk_device);
void terminate_compute_room_texture(void);

void compute_room_texture(const room_t room, const uint32_t cacheSlot, const Texture tilemapTexture, Texture *const pRoomTexture);

#endif	// COMPUTE_ROOM_TEXTURE_H
