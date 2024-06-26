#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <stdint.h>

#include "game/area/room.h"
#include "render/texture_info.h"
#include "render/texture_state.h"
#include "util/string.h"

#include "texture.h"

extern const uint32_t num_textures;
extern const texture_handle_t missing_texture_handle;
extern const texture_handle_t room_texture_handle;

void load_textures(const texture_pack_t texture_pack);
void create_room_texture(const room_t room, const uint32_t cache_slot, const texture_handle_t tilemap_texture_handle);
void destroy_textures(void);

texture_t get_room_texture(const room_size_t room_size);
texture_t get_loaded_texture(const texture_handle_t texture_handle);
texture_handle_t find_loaded_texture_handle(const string_t texture_id);

texture_state_t missing_texture_state(void);
texture_state_t make_new_texture_state(const texture_handle_t texture_handle);

#endif	// TEXTURE_MANAGER_H
