#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "game/area/room.h"

#include "../texture.h"

void init_compute_room_texture(void);

void terminate_compute_room_texture(void);

texture_t init_room_texture(const room_size_t room_size, const uint32_t cache_slot);

void compute_room_texture(const texture_t tilemap_texture, const room_size_t room_size, const uint32_t *tile_data, texture_t *const room_texture_ptr);

#endif	// COMPUTE_ROOM_TEXTURE_H
