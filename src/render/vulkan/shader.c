#include "shader.h"

#define __STDC_WANT_LIB_EXT1__ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"

#define SHADER_DIRECTORY (RESOURCE_PATH "shaders/")

static const char *shader_directory = SHADER_DIRECTORY;

static const uint32_t initial_buffer_size = 8192;

char *read_file(const char *path, uint32_t *file_size_ptr) {

	// TODO - use safe "_s" io functions

	log_message(VERBOSE, "Reading shader file...");
	logf_message(VERBOSE, "Opening shader file at \"%s\"...", path);

	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		log_message(ERROR, "Error reading shader file: could not open file.");
		return NULL;
	}

	char *contents = calloc(initial_buffer_size, sizeof(char));
	if (contents == NULL) {
		log_message(ERROR, "Error reading shader file: could not allocate buffer for shader bytecode.");
		return NULL;
	}

	// TODO - make this able to process larger shader files.
	uint32_t file_size = 0;
	char next_char = fgetc(file);
	while (next_char != EOF) {
		contents[file_size++] = next_char;
		next_char = fgetc(file);
	}

	fclose(file);

	if (file_size_ptr != NULL) {
		*file_size_ptr = file_size;
	}
	else {
		log_message(WARNING, "Warning reading shader file: pointer to return file size is NULL.");
	}

	return contents;
}

void create_shader_module(VkDevice device, const char *filename, VkShaderModule *shader_module_ptr) {

	logf_message(VERBOSE, "Loading shader \"%s\"...", filename);

	if (filename == NULL) {
		log_message(ERROR, "Error creating shader module: pointer to filename is NULL.");
		return;
	}

	if (shader_module_ptr == NULL) {
		log_message(ERROR, "Error creating shader module: pointer to shader module is NULL");
		return;
	}

	char path[256];
	memset(path, '\0', 256);

	// Length does not include null-terminator.
	const size_t filename_length = strnlen_s(filename, 64); // Limit to 64 characters.
	const size_t shader_directory_length = strnlen_s(SHADER_DIRECTORY, 191); // Limit to 256 - 64 - 1 characters, to make room for filename and for null-terminator.

	const errno_t shader_directory_strncpy_result = strncpy_s(path, 256, shader_directory, shader_directory_length);
	if (shader_directory_strncpy_result != 0) {
		char error_message[256];
		const errno_t strerror_result = strerror_s(error_message, 256, shader_directory_strncpy_result);
		if (strerror_result == 0) {
			logf_message(ERROR, "Error creating shader module: failed to copy shader directory into shader path buffer. (Error code: \"%s\")", error_message);
		}
		else {
			log_message(WARNING, "Failed to get error message.");
			log_message(ERROR, "Error creating shader module: failed to copy shader directory into shader path buffer.");
		}
		return;
	}

	const errno_t filename_strncat_result = strncat_s(path, 256, filename, filename_length); 
	if (filename_strncat_result != 0) {
		char error_message[256];
		const errno_t strerror_result = strerror_s(error_message, 256, filename_strncat_result);
		if (strerror_result == 0) {
			logf_message(ERROR, "Error creating shader module: failed to copy shader filename into shader path buffer. (Error code: \"%s\")", error_message);
		}
		else {
			log_message(WARNING, "Failed to get error message.");
			log_message(ERROR, "Error creating shader module: failed to copy shader filename into shader path buffer.");
		}
		return;
	}

	uint32_t file_size = 0;
	char *bytecode = read_file(path, &file_size);

	if (bytecode == NULL) {
		log_message(FATAL, "Fatal error creating shader module: shader file reading failed.");
		return;
	}

	VkShaderModuleCreateInfo create_info = { 0 };
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.codeSize = file_size;
	create_info.pCode = (uint32_t *)bytecode;

	VkResult result = vkCreateShaderModule(device, &create_info, NULL, shader_module_ptr);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Fatal error creating shader module: shader module creation failed. (Error code: %i)", result);
	}

	free(bytecode);
}
