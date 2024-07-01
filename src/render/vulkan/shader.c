#include "shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/logging.h"
#include "util/allocate.h"
#include "util/byte.h"

#define SHADER_DIRECTORY (RESOURCE_PATH "shaders/")

typedef struct ShaderBytecode {
	uint32_t bytecode_size;
	byte_t *bytecode;
} ShaderBytecode;

static const char *shader_directory = SHADER_DIRECTORY;
static const uint32_t initial_buffer_size = 8192;

static ShaderBytecode read_shader_file(const char *const path) {
	logMsgF(LOG_LEVEL_VERBOSE, "Reading shader file at \"%s\"...", path);
	
	ShaderBytecode shader_bytecode = {
		.bytecode_size = 0,
		.bytecode = nullptr
	};
	
	if (path == nullptr) {
		logMsg(LOG_LEVEL_ERROR, "Error reading shader file: pointer to path string is null.");
		return shader_bytecode;
	}

	FILE *file = fopen(path, "rb");
	if (file == nullptr) {
		logMsg(LOG_LEVEL_ERROR, "Error reading shader file: could not open file.");
		return shader_bytecode;
	}
	
	while (fgetc(file) != EOF) {
		++shader_bytecode.bytecode_size;
	}
	
	if (!allocate((void **)&shader_bytecode.bytecode, initial_buffer_size, sizeof(byte_t))) {
		logMsg(LOG_LEVEL_ERROR, "Error reading shader file: failed to allocate buffer for shader bytecode.");
		return shader_bytecode;
	}

	uint32_t file_size = 0;
	rewind(file);
	char next_char = fgetc(file);
	while (next_char != EOF) {
		shader_bytecode.bytecode[file_size++] = next_char;
		next_char = fgetc(file);
	}

	fclose(file);
	return shader_bytecode;
}

static bool destroy_shader_bytecode(ShaderBytecode *const pShaderBytecode) {
	if (pShaderBytecode == nullptr) {
		return false;
	}
	
	deallocate((void **)&pShaderBytecode->bytecode);
	pShaderBytecode->bytecode_size = 0;
	
	return true;
}

shader_module_t create_shader_module(VkDevice device, const char *const filename) {
	logMsgF(LOG_LEVEL_VERBOSE, "Loading shader \"%s\"...", filename);

	shader_module_t shader_module = {
		.module_handle = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE
	};

	if (filename == nullptr) {
		logMsg(LOG_LEVEL_ERROR, "Error creating shader module: pointer to filename is nullptr.");
		return shader_module;
	}

	char path[256];
	memset(path, '\0', 256);

	// Length does not include null-terminator.
	const size_t filename_length = strlen(filename); // Limit to 64 characters.
	const size_t shader_directory_length = strlen(SHADER_DIRECTORY); // Limit to 256 - 64 - 1 characters, to make room for filename and for null-terminator.
	strncpy(path, shader_directory, shader_directory_length);
	strncat(path, filename, filename_length);

	ShaderBytecode shader_bytecode = read_shader_file(path);
	if (shader_bytecode.bytecode == nullptr) {
		logMsg(LOG_LEVEL_FATAL, "Fatal error creating shader module: shader file reading failed.");
		return shader_module;
	}

	const VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = shader_bytecode.bytecode_size,
		.pCode = (uint32_t *)shader_bytecode.bytecode
	};

	const VkResult result = vkCreateShaderModule(device, &create_info, nullptr, &shader_module.module_handle);
	if (result != VK_SUCCESS) {
		logMsgF(LOG_LEVEL_FATAL, "Fatal error creating shader module: shader module creation failed (error code: %i).", result);
		return shader_module;
	}
	shader_module.device = device;

	destroy_shader_bytecode(&shader_bytecode);
	return shader_module;
}

bool destroy_shader_module(shader_module_t *const shader_module_ptr) {
	
	if (shader_module_ptr == nullptr) {
		return false;
	}
	
	vkDestroyShaderModule(shader_module_ptr->device, shader_module_ptr->module_handle, nullptr);
	shader_module_ptr->module_handle = VK_NULL_HANDLE;
	shader_module_ptr->device = VK_NULL_HANDLE;
	
	return true;
}