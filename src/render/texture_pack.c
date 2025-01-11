#include "texture_pack.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "log/Logger.h"
#include "util/Allocation.h"
#include "util/FileIO.h"

void deleteTexturePack(TexturePack *const pTexturePack) {
	assert(pTexturePack);
	for (uint32_t i = 0; i < pTexturePack->numTextures; ++i) {
		deleteString(&pTexturePack->pTextureCreateInfos[i].textureID);
		pTexturePack->pTextureCreateInfos[i].animations = heapFree(pTexturePack->pTextureCreateInfos[i].animations);
	}
	heapFree(pTexturePack->pTextureCreateInfos);
	pTexturePack->pTextureCreateInfos = nullptr;
}

TexturePack readTexturePackFile(const char *pPath) {
	assert(pPath);
	logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Loading texture pack from \"%s\"...", pPath);

	TexturePack texturePack = { };
	File file = openFile(pPath, FMODE_READ, FMODE_NO_UPDATE, FMODE_BINARY);

	static const char fgt_label[4] = "FGT";
	char label[4];
	fread(label, 1, 4, file.pStream);
	if (strcmp(label, fgt_label) != 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Invalid file format; found label \"%s\".", label);
		goto end_read;
	}

	fileReadData(file, sizeof(uint32_t), 1, &texturePack.numTextures);

	if (texturePack.numTextures == 0) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Number of textures specified as zero.");
		goto end_read;
	}

	texturePack.pTextureCreateInfos = heapAlloc(texturePack.numTextures, sizeof(TextureCreateInfo));
	if (!texturePack.pTextureCreateInfos) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Texture create info array allocation failed.");
		goto end_read;
	}

	for (uint32_t i = 0; i < texturePack.numTextures; ++i) {
		TextureCreateInfo *pTextureInfo = &texturePack.pTextureCreateInfos[i];

		// Read texture ID.
		pTextureInfo->textureID = fileReadString(file, 256);
		if (stringIsNull(pTextureInfo->textureID)) {
			logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading texture pack: failed to read texture ID.");
			goto end_read;
		}

		// Read texture type.
		uint32_t textureCreateInfoFlags = 0;
		fileReadData(file, sizeof(uint32_t), 1, &textureCreateInfoFlags);
		if (textureCreateInfoFlags & 0x00000001U) pTextureInfo->isLoaded = true;
		if (textureCreateInfoFlags & 0x00000002U) pTextureInfo->isTilemap = true;

		// Read number of cells in texture atlas.
		fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->numCells.width);
		fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->numCells.length);

		// Read texture cell extent.
		fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->cellExtent.width);
		fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->cellExtent.length);

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
		fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->numAnimations);
		if (pTextureInfo->numAnimations > 0) {

			pTextureInfo->animations = heapAlloc(pTextureInfo->numAnimations, sizeof(TextureAnimation));
			if (!pTextureInfo->animations) {
				logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading texture file: failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			for (uint32_t j = 0; j < pTextureInfo->numAnimations; ++j) {
				fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->animations[j].startCell);
				fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->animations[j].numFrames);
				fileReadData(file, sizeof(uint32_t), 1, &pTextureInfo->animations[j].framesPerSecond);
			}
		} else {
			// If there are no specified animations, then set the number of animations to one 
			// 	and set the first (and only) animation to a default value.
			// This eliminates the need for branching when querying animation cycles in a texture.
			pTextureInfo->numAnimations = 1;
			pTextureInfo->animations = heapAlloc(pTextureInfo->numAnimations, sizeof(TextureAnimation));
			if (!pTextureInfo->animations) {
				logMsg(loggerSystem, LOG_LEVEL_ERROR, "Error reading texture file: failed to allocate array of animation create infos in texture %u.", i);
				goto end_read;
			}

			pTextureInfo->animations[0].startCell = 0;
			pTextureInfo->animations[0].numFrames = 1;
			pTextureInfo->animations[0].framesPerSecond = 0;
		}
	}

end_read:
	closeFile(&file);
	return texturePack;
}
