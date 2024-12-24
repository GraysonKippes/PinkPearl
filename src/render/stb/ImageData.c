#include "ImageData.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include "log/Logger.h"

ImageData loadImageData(const char *const pPath, const size_t numChannels) {
	
	ImageData imageData = { };
	
	if  (numChannels == 0) {
		imageData.pPixels = stbi_load(pPath, (int *)&imageData.width, (int *)&imageData.height, (int *)&imageData.numChannels, 0);
	} else {
		imageData.pPixels = stbi_load(pPath, (int *)&imageData.width, (int *)&imageData.height, nullptr, (int)numChannels);
		imageData.numChannels = numChannels;
	}
	
	if (!imageData.pPixels) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Loading image: failed to load image \"%s\".", pPath);
		return (ImageData){ };
	}

	return imageData;
}

void deleteImageData(ImageData *const pImageData) {
	assert(pImageData);
	stbi_image_free(pImageData->pPixels);
	pImageData->pPixels = nullptr;
	pImageData->width = 0;
	pImageData->height = 0;
	pImageData->numChannels = 0;
}
