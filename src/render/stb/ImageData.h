#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <stddef.h>

#define GRAYSCALE				1LLU
#define GRAYSCALE_TRANSPARENT	2LLU
#define COLOR					3LLU
#define COLOR_TRANSPARENT		4LLU

typedef struct ImageData {
	
	// The data of this image.
	unsigned char *pPixels;

	// The width of this image, in pixels.
	size_t width;

	// The height of this image, in pixels.
	size_t height;

	// The number of components in each pixel.
	// 1 = Grayscale;
	// 2 = Grayscale and Alpha;
	// 3 = Red, Green, Blue; and
	// 4 = Red, Green, Blue, and Alpha.
	size_t numChannels;

} ImageData;

// Loads an image at the given filepath.
ImageData loadImageData(const char *const pPath, const size_t numChannels);

// Frees an image, destroying the data buffer.
void deleteImageData(ImageData *const pImageData);

#endif	// IMAGE_DATA_H
