#ifndef IMAGE_DATA_H
#define IMAGE_DATA_H

#include <stddef.h>
#include <stdint.h>

#include <GLFW/glfw3.h>

#include "util/byte.h"

// TODO - change integer types to uint32_t.
typedef struct image_data_t {
	
	// The data of this image.
	byte_t *data;

	// The width of this image, in pixels.
	size_t width;

	// The height of this image, in pixels.
	size_t height;

	// The number of components in each pixel.
	// 1 = Grayscale;
	// 2 = Grayscale and Alpha;
	// 3 = Red, Green, Blue; and
	// 4 = Red, Green, Blue, and Alpha.
	size_t num_channels;

} image_data_t;

// Loads an image at the given filepath.
image_data_t load_image_data(const char *path, int num_channels);

// Frees an image, destroying the data buffer.
void free_image_data(image_data_t image);

GLFWimage load_glfw_image(const char *path);

#endif	// IMAGE_DATA_H
