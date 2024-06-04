#include "texture_info.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/error_code.h"
#include "log/log_stack.h"
#include "log/logging.h"
#include "util/allocate.h"
#include "util/file_io.h"

void deleteTexturePack(TexturePack *const pTexturePack) {
	if (pTexturePack == NULL) {
		return;
	}

	for (uint32_t i = 0; i < pTexturePack->num_textures; ++i) {
		deleteString(&pTexturePack->texture_create_infos[i].textureID);
		deallocate((void **)&pTexturePack->texture_create_infos[i].animations);
	}

	free(pTexturePack->texture_create_infos);
	pTexturePack->texture_create_infos = NULL;
}

TexturePack parse_fgt_file(const char *path) {
	log_stack_push("LoadTexturePack");
	logf_message(VERBOSE, "Loading texture pack from \"%s\"...", path);

	TexturePack texturePack = { 0 };
	texturePack.num_textures = 0;
	texturePack.texture_create_infos = NULL;

	if (path == NULL) {
		log_message(ERROR, "Filename is NULL.");
		return texturePack;
	}

	FILE *fgt_file = fopen(path, "rb");
	if (fgt_file == NULL) {
		logf_message(ERROR, "File not found at \"%s\".", path);
		return texturePack;
	}

	static const char fgt_label[4] = "FGT";
	char label[4];
	fread(label, 1, 4, fgt_file);
	if (strcmp(label, fgt_label) != 0) {
		logf_message(ERROR, "Invalid file format; found label \"%s\".", label);
		goto end_read;
	}

	read_data(fgt_file, sizeof(uint32_t), 1, &texturePack.num_textures);

	if (texturePack.num_textures == 0) {
		log_message(ERROR, "Number of textures specified as zero.");
		goto end_read;
	}

	if (!allocate((void **)&texturePack.texture_create_infos, texturePack.num_textures, sizeof(TextureCreateInfo))) {
		log_message(ERROR, "Texture create info array allocation failed.");
		goto end_read;
	}

	for (uint32_t i = 0; i < texturePack.num_textures; ++i) {
		TextureCreateInfo *pTextureInfo = &texturePack.texture_create_infos[i];

		// Read texture ID.
		pTextureInfo->textureID = readString(fgt_file, 256);
		if (stringIsNull(pTextureInfo->textureID)) {
			log_message(ERROR, "Error reading texture pack: failed to read texture ID.");
			goto end_read;
		}

		// Read texture type.
		uint32_t textureCreateInfoFlags = 0;
		read_data(fgt_file, sizeof(uint32_t), 1, &textureCreateInfoFlags);
		
		if (textureCreateInfoFlags & 0x00000001) {
			pTextureInfo->isLoaded = true;
		}
		if (textureCreateInfoFlags & 0x00000002) {
			pTextureInfo->isTilemap = true;
		}

		// Read number of cells in texture atlas.
		read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->numCells.width);
		read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->numCells.length);

		// Read texture cell extent.
		read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->cellExtent.width);
		read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->cellExtent.length);

		// Check extents -- if any of them are zero, then there certainly was an error.
		if (pTextureInfo->numCells.width == 0) {
			log_message(WARNING, "Texture create info number of cells widthwise is zero.");
		}
		if (pTextureInfo->numCells.length == 0) {
			log_message(WARNING, "Texture create info number of cells lengthwise is zero.");
		}
		if (pTextureInfo->cellExtent.width == 0) {
			log_message(WARNING, "Texture create info cell extent width is zero.");
		}
		if (pTextureInfo->cellExtent.length == 0) {
			log_message(WARNING, "Texture create info cell extent length is zero.");
		}

		// Read animation create infos.
		read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->num_animations);
		if (pTextureInfo->num_animations > 0) {

			if (!allocate((void **)&pTextureInfo->animations, pTextureInfo->num_animations, sizeof(animation_create_info_t))) {
				logf_message(ERROR, "Failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			for (uint32_t j = 0; j < pTextureInfo->num_animations; ++j) {
				read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->animations[j].start_cell);
				read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->animations[j].num_frames);
				read_data(fgt_file, sizeof(uint32_t), 1, &pTextureInfo->animations[j].frames_per_second);
			}
		}
		else {

			// If there are no specified animations, then set the number of animations to one 
			// 	and set the first (and only) animation to a default value.
			// This eliminates the need for branching when querying animation cycles in a texture.
			pTextureInfo->num_animations = 1;
			if (!allocate((void **)&pTextureInfo->animations, pTextureInfo->num_animations, sizeof(animation_create_info_t))) {
				logf_message(ERROR, "Error reading texture file: failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			pTextureInfo->animations[0].start_cell = 0;
			pTextureInfo->animations[0].num_frames = 1;
			pTextureInfo->animations[0].frames_per_second = 0;
		}
	}

end_read:
	fclose(fgt_file);
	error_queue_flush();
	log_stack_pop();
	return texturePack;
}
