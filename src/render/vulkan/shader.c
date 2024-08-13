#include "Shader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "log/Logger.h"
#include "util/allocate.h"
#include "util/byte.h"

#define SHADER_DIRECTORY (RESOURCE_PATH "shaders/")

static const char shaderEntrypoint[] = "main";

typedef struct ShaderBytecode {
	uint32_t bytecode_size;
	byte_t *bytecode;
} ShaderBytecode;

static const char *shader_directory = SHADER_DIRECTORY;
static const uint32_t initial_buffer_size = 8192;

static ShaderBytecode read_shader_file(const char *const pPath) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Reading shader file at \"%s\"...", pPath);
	
	ShaderBytecode shader_bytecode = {
		.bytecode_size = 0,
		.bytecode = nullptr
	};
	
	if (pPath == nullptr) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error reading shader file: pointer to pPath string is null.");
		return shader_bytecode;
	}

	FILE *file = fopen(pPath, "rb");
	if (file == nullptr) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error reading shader file: could not open file.");
		return shader_bytecode;
	}
	
	while (fgetc(file) != EOF) {
		++shader_bytecode.bytecode_size;
	}
	
	if (!allocate((void **)&shader_bytecode.bytecode, initial_buffer_size, sizeof(byte_t))) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error reading shader file: failed to allocate buffer for shader bytecode.");
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

ShaderModule createShaderModule(const VkDevice vkDevice, const ShaderStage shaderStage, const char *const pFilename) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating shader module...");

	ShaderModule shaderModule = {
		.stage = shaderStage,
		.vkShaderModule = VK_NULL_HANDLE,
		.vkDevice = VK_NULL_HANDLE
	};

	if (!pFilename) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating shader module: pFilename is null.");
		return shaderModule;
	}
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Loading shader source \"%s\"...", pFilename);

	char path[256];
	memset(path, '\0', 256);

	// Length does not include null-terminator.
	const size_t filename_length = strlen(pFilename); // Limit to 64 characters.
	const size_t shader_directory_length = strlen(SHADER_DIRECTORY); // Limit to 256 - 64 - 1 characters, to make room for pFilename and for null-terminator.
	strncpy(path, shader_directory, shader_directory_length);
	strncat(path, pFilename, filename_length);

	ShaderBytecode shader_bytecode = read_shader_file(path);
	if (!shader_bytecode.bytecode) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating shader module: shader file reading failed.");
		return shaderModule;
	}

	const VkShaderModuleCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.codeSize = shader_bytecode.bytecode_size,
		.pCode = (uint32_t *)shader_bytecode.bytecode
	};

	const VkResult result = vkCreateShaderModule(vkDevice, &create_info, nullptr, &shaderModule.vkShaderModule);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating shader module: shader module creation failed (error code: %i).", result);
		return shaderModule;
	}
	shaderModule.vkDevice = vkDevice;

	destroy_shader_bytecode(&shader_bytecode);
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Created shader module.");
	return shaderModule;
}

bool destroyShaderModule(ShaderModule *const pShaderModule) {
	if (!pShaderModule) {
		return false;
	}
	
	vkDestroyShaderModule(pShaderModule->vkDevice, pShaderModule->vkShaderModule, nullptr);
	pShaderModule->vkShaderModule = VK_NULL_HANDLE;
	pShaderModule->vkDevice = VK_NULL_HANDLE;
	
	return true;
}

VkPipelineShaderStageCreateInfo makeShaderStageCreateInfo(const ShaderModule shaderModule) {
	
	VkShaderStageFlags vkShaderStageFlags = VK_SHADER_STAGE_ALL;
	switch (shaderModule.stage) {
		case SHADER_STAGE_VERTEX:
			vkShaderStageFlags = VK_SHADER_STAGE_VERTEX_BIT; 
			break;
		case SHADER_STAGE_TESSELLATION_CONTROL:
			vkShaderStageFlags = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; 
			break;
		case SHADER_STAGE_TESSELLATION_EVALUATION:
			vkShaderStageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT; 
			break;
		case SHADER_STAGE_GEOMETRY:
			vkShaderStageFlags = VK_SHADER_STAGE_GEOMETRY_BIT; 
			break;
		case SHADER_STAGE_FRAGMENT:
			vkShaderStageFlags = VK_SHADER_STAGE_FRAGMENT_BIT; 
			break;
		case SHADER_STAGE_COMPUTE:
			vkShaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT; 
			break;
		case SHADER_STAGE_TASK:
			vkShaderStageFlags = VK_SHADER_STAGE_TASK_BIT_EXT; 
			break;
		case SHADER_STAGE_MESH:
			vkShaderStageFlags = VK_SHADER_STAGE_MESH_BIT_EXT; 
			break;
		default:
			logMsg(loggerVulkan,LOG_LEVEL_ERROR, "Error making shader stage create info: invalid shader stage (%i).", (int)shaderModule.stage);
			break;
	}
	
	return (VkPipelineShaderStageCreateInfo){
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = vkShaderStageFlags,
		.module = shaderModule.vkShaderModule,
		.pName = shaderEntrypoint
	};
}