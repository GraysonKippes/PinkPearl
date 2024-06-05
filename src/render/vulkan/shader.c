#include "shader.h"

#define __STDC_WANT_LIB_EXT1__ 1

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
	logf_message(VERBOSE, "Reading shader file at \"%s\"...", path);
	
	ShaderBytecode shader_bytecode = {
		.bytecode_size = 0,
		.bytecode = nullptr
	};
	
	if (path == nullptr) {
		log_message(ERROR, "Error reading shader file: pointer to path string is null.");
		return shader_bytecode;
	}

	FILE *file = fopen(path, "rb");
	if (file == nullptr) {
		log_message(ERROR, "Error reading shader file: could not open file.");
		return shader_bytecode;
	}
	
	while (fgetc(file) != EOF) {
		++shader_bytecode.bytecode_size;
	}
	
	if (!allocate((void **)&shader_bytecode.bytecode, initial_buffer_size, sizeof(byte_t))) {
		log_message(ERROR, "Error reading shader file: failed to allocate buffer for shader bytecode.");
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
	logf_message(VERBOSE, "Loading shader \"%s\"...", filename);

	shader_module_t shader_module = {
		.module_handle = VK_NULL_HANDLE,
		.device = VK_NULL_HANDLE
	};

	if (filename == nullptr) {
		log_message(ERROR, "Error creating shader module: pointer to filename is nullptr.");
		return shader_module;
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
		return shader_module;
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
		return shader_module;
	}

	ShaderBytecode shader_bytecode = read_shader_file(path);
	if (shader_bytecode.bytecode == nullptr) {
		log_message(FATAL, "Fatal error creating shader module: shader file reading failed.");
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
		logf_message(FATAL, "Fatal error creating shader module: shader module creation failed (error code: %i).", result);
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