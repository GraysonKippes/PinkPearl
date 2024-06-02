#ifndef VK_TEXTURE_LOADER_H
#define VK_TEXTURE_LOADER_H

#include "render/texture_info.h"

#include "texture.h"

void create_image_staging_buffer(void);
void destroy_image_staging_buffer(void);

Texture loadTexture(const TextureCreateInfo texture_create_info);

#endif	// VK_TEXTURE_LOADER_H
