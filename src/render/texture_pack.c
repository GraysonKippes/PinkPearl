#include "texture_pack.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/Logger.h"
#include "util/allocate.h"
#include "util/file_io.h"

void deleteTexturePack(TexturePack *const pTexturePack) {
	if (!pTexturePack) {
		return;
	}

	for (uint32_t i = 0; i < pTexturePack->numTextures; ++i) {
		deleteString(&pTexturePack->pTextureCreateInfos[i].textureID);
		deallocate((void **)&pTexturePack->pTextureCreateInfos[i].animations);
	}

	free(pTexturePack->pTextureCreateInfos);
	pTexturePack->pTextureCreateInfos = nullptr;
}

TexturePack readTexturePackFile(const char *pPath) {
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Loading texture pack from \"%s\"...", pPath);

	TexturePack texturePack = {
		.numTextures = 0,
		.pTextureCreateInfos = nullptr
	};

	if (!pPath) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Filename is nullptr.");
		return texturePack;
	}

	FILE *pFile = fopen(pPath, "rb");
	if (!pFile) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "File not found at \"%s\".", pPath);
		return texturePack;
	}

	static const char fgt_label[4] = "FGT";
	char label[4];
	fread(label, 1, 4, pFile);
	if (strcmp(label, fgt_label) != 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Invalid file format; found label \"%s\".", label);
		goto end_read;
	}

	read_data(pFile, sizeof(uint32_t), 1, &texturePack.numTextures);

	if (texturePack.numTextures == 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Number of textures specified as zero.");
		goto end_read;
	}

	if (!allocate((void **)&texturePack.pTextureCreateInfos, texturePack.numTextures, sizeof(TextureCreateInfo))) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Texture create info array allocation failed.");
		goto end_read;
	}

	for (uint32_t i = 0; i < texturePack.numTextures; ++i) {
		TextureCreateInfo *pTextureInfo = &texturePack.pTextureCreateInfos[i];

		// Read texture ID.
		pTextureInfo->textureID = readString(pFile, 256);
		if (stringIsNull(pTextureInfo->textureID)) {
			logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading texture pack: failed to read texture ID.");
			goto end_read;
		}

		// Read texture type.
		uint32_t textureCreateInfoFlags = 0;
		read_data(pFile, sizeof(uint32_t), 1, &textureCreateInfoFlags);
		
		if (textureCreateInfoFlags & 0x00000001) {
			pTextureInfo->isLoaded = true;
		}
		if (textureCreateInfoFlags & 0x00000002) {
			pTextureInfo->isTilemap = true;
		}

		// Read number of cells in texture atlas.
		read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->numCells.width);
		read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->numCells.length);

		// Read texture cell extent.
		read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->cellExtent.width);
		read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->cellExtent.length);

		// Check extents -- if any of them are zero, then there certainly was an error.
		if (pTextureInfo->numCells.width == 0) {
			logMsg(loggerSystem, LOG_LEVEL_WARNING, "Texture create info number of cells widthwise is zero.");
		}
		if (pTextureInfo->numCells.length == 0) {
			logMsg(loggerSystem, LOG_LEVEL_WARNING, "Texture create info number of cells lengthwise is zero.");
		}
		if (pTextureInfo->cellExtent.width == 0) {
			logMsg(loggerSystem, LOG_LEVEL_WARNING, "Texture create info cell extent width is zero.");
		}
		if (pTextureInfo->cellExtent.length == 0) {
			logMsg(loggerSystem, LOG_LEVEL_WARNING, "Texture create info cell extent length is zero.");
		}

		// Read animation create infos.
		read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->numAnimations);
		if (pTextureInfo->numAnimations > 0) {

			if (!allocate((void **)&pTextureInfo->animations, pTextureInfo->numAnimations, sizeof(TextureAnimation))) {
				logMsg(loggerSystem, LOG_LEVEL_ERROR, "Failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			for (uint32_t j = 0; j < pTextureInfo->numAnimations; ++j) {
				read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->animations[j].startCell);
				read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->animations[j].numFrames);
				read_data(pFile, sizeof(uint32_t), 1, &pTextureInfo->animations[j].framesPerSecond);
			}
		}
		else {

			// If there are no specified animations, then set the number of animations to one 
			// 	and set the first (and only) animation to a default value.
			// This eliminates the need for branching when querying animation cycles in a texture.
			pTextureInfo->numAnimations = 1;
			if (!allocate((void **)&pTextureInfo->animations, pTextureInfo->numAnimations, sizeof(TextureAnimation))) {
				logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading texture file: failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			pTextureInfo->animations[0].startCell = 0;
			pTextureInfo->animations[0].numFrames = 1;
			pTextureInfo->animations[0].framesPerSecond = 0;
		}
	}

end_read:
	fclose(pFile);
	return texturePack;
}
