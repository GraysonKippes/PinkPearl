#include "image_data.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "log/Logger.h"

image_data_t load_image_data(const char *path, int num_channels) {
	
	image_data_t image_data = { };
	image_data.data = stbi_load(path, (int *)&image_data.width, (int *)&image_data.height, (int *)&image_data.num_channels, num_channels);

	if (image_data.data == nullptr) {
		logMsg(loggerSystem, LOG_LEVEL_ERROR, "Image failed to load. (Path: \"%s\")", path);
	}

	return image_data;
}

void free_image_data(image_data_t image_data) {
	stbi_image_free(image_data.data);
}

GLFWimage load_glfw_image(const char *path) {
	
	GLFWimage image = { };
	stbi_load(path, &image.width, &image.height, nullptr, 4);

	return image;
}

void free_glfw_image(GLFWimage image) {
	stbi_image_free(image.pixels);
}
