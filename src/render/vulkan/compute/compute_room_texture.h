#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "game/area/room.h"

#include "../texture.h"

void init_compute_room_texture(const VkDevice vk_device);
void terminate_compute_room_texture(void);

texture_t init_room_texture(const room_size_t room_size, const uint32_t cache_slot);

void compute_room_texture(const room_t room, const uint32_t cache_slot, const texture_t tilemap_texture, texture_t *const room_texture_ptr);

#endif	// COMPUTE_ROOM_TEXTURE_H
