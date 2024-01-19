#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <stdint.h>

#include "render/texture_info.h"

#include "texture.h"

void load_textures(const texture_pack_t texture_pack);

void destroy_textures(void);

texture_t get_loaded_texture(uint32_t texture_index);

texture_t find_loaded_texture(const char *const name);

#endif	// TEXTURE_MANAGER_H
