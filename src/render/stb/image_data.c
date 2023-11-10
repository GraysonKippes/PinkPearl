#include "image_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "log/logging.h"

image_data_t load_image_data(const char *path) {
	
	image_data_t image_data = { 0 };
	image_data.m_data = stbi_load(path, (int *)&image_data.m_width, (int *)&image_data.m_height, (int *)&image_data.m_num_channels, 0);

	if (image_data.m_data == NULL) {
		logf_message(ERROR, "Image failed to load. (Path: \"%s\")", path);
	}

	return image_data;
}

void free_image_data(image_data_t image_data) {
	stbi_image_free(image_data.m_data);
}
