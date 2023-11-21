#ifndef COMPUTE_ROOM_TEXTURE_H
#define COMPUTE_ROOM_TEXTURE_H

#include <stdint.h>

#include "../texture.h"

void init_compute_room_texture(void);

void terminate_compute_room_texture(void);

void compute_room_texture(texture_t tilemap_texture, uint32_t cache_slot, extent_t room_extent, uint32_t *tile_data);

texture_t get_room_texture(uint32_t room_size, uint32_t room_texture_cache_slot);

#endif	// COMPUTE_ROOM_TEXTURE_H
