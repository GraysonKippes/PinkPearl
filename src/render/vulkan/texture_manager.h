#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include <stdint.h>

#include "texture.h"

void load_textures(void);

void destroy_textures(void);

texture_t get_loaded_texture(uint32_t texture_index);

#endif	// TEXTURE_MANAGER_H
