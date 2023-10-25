#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

image_t load_image(const char *path) {
	
	image_t image;
	image.m_data = stbi_load(path, &image.m_width, &image.m_height, &image.m_num_channels, 0);

	if (image.m_data == NULL) {
		// TODO - error handling
	}

	return image;
}

void free_image(image_t image) {
	stbi_image_free(image.m_data);
}
