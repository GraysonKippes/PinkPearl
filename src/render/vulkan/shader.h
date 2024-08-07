#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

typedef enum ShaderStage {
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_TESSELATION_CONTROL,
	SHADER_STAGE_TESSELATION_EVALUATION,
	SHADER_STAGE_GEOMETRY,
	SHADER_STAGE_FRAGMENT,
	SHADER_STAGE_COMPUTE,
	SHADER_STAGE_TASK,
	SHADER_STAGE_MESH
} ShaderStage;

typedef struct ShaderModule {
	VkShaderModule module_handle;
	VkDevice device;
} ShaderModule;

ShaderModule create_shader_module(const VkDevice device, const char *const pFilename);

bool destroy_shader_module(ShaderModule *const shader_module_ptr);

#endif	// SHADER_H
