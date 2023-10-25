#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log/logging.h"

#define OS_WINDOWS

#ifdef OS_UNIX
static const size_t shader_directory_length = 22;	// Length includes null-terminator.

static const char *shader_directory = "../resources/shaders/";
#endif

#ifdef OS_WINDOWS
static const size_t shader_directory_length = 25;	// Length includes null-terminator.

static const char *shader_directory = "../../resources/shaders/";
#endif

static const uint32_t initial_buffer_size = 8192;

char *read_file(const char *path, uint32_t *file_size_ptr) {

	logf_message(VERBOSE, "Opening shader file at \"%s\"", path);

	FILE *file = fopen(path, "rb");
	if (file == NULL)
		return NULL;

	char *contents = calloc(initial_buffer_size, sizeof(char));
	if (contents == NULL)
		return NULL;

	// TODO - make this able to process larger shader files.
	uint32_t file_size = 0;
	char next_char = fgetc(file);
	while (next_char != EOF) {
		contents[file_size++] = next_char;
		next_char = fgetc(file);
	}

	fclose(file);

	if (file_size_ptr != NULL)
		*file_size_ptr = file_size;

	return contents;
}

void create_shader_module(VkDevice logical_device, const char *filename, VkShaderModule *shader_module_ptr) {

	logf_message(INFO, "Loading shader \"%s\"...", filename);

	size_t filename_length = strlen(filename);	// Length does not include null-terminator.
	size_t path_length = shader_directory_length + filename_length;
	char *path = calloc(path_length, sizeof(char));
	strcpy(path, shader_directory);
	strcpy(path + shader_directory_length - 1, filename);


	uint32_t file_size = 0;
	char *bytecode = read_file(path, &file_size);
	free(path);

	if (bytecode == NULL) {
		log_message(FATAL, "Shader file reading failed.");
	}

	VkShaderModuleCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.codeSize = file_size;
	create_info.pCode = (uint32_t *)bytecode;

	VkResult result = vkCreateShaderModule(logical_device, &create_info, NULL, shader_module_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Shader module creation failed. (Error code: %i)", result);
	}

	free(bytecode);
}
