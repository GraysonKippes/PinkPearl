#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

typedef enum ShaderStage {
	SHADER_STAGE_VERTEX,
	SHADER_STAGE_TESSELLATION_CONTROL,
	SHADER_STAGE_TESSELLATION_EVALUATION,
	SHADER_STAGE_GEOMETRY,
	SHADER_STAGE_FRAGMENT,
	SHADER_STAGE_COMPUTE,
	SHADER_STAGE_TASK,
	SHADER_STAGE_MESH
} ShaderStage;

typedef struct ShaderModule {
	
	ShaderStage stage;
	
	VkShaderModule vkShaderModule;
	
	VkDevice vkDevice;
	
} ShaderModule;

ShaderModule createShaderModule(const VkDevice vkDevice, const ShaderStage shaderStage, const char *const pFilename);

bool destroyShaderModule(ShaderModule *const pShaderModule);

VkPipelineShaderStageCreateInfo makeShaderStageCreateInfo(const ShaderModule shaderModule);

#endif	// SHADER_H
