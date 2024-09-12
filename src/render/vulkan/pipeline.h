#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>

typedef enum PipelineType {
	PIPELINE_TYPE_GRAPHICS,
	PIPELINE_TYPE_COMPUTE
} PipelineType;

typedef struct Pipeline {
	
	PipelineType type;

	VkPipeline vkPipeline;
	VkPipelineLayout vkPipelineLayout;
	
	VkDescriptorPool vkDescriptorPool;
	VkDescriptorSetLayout vkDescriptorSetLayout;
	
	VkDevice vkDevice;

} Pipeline;

typedef struct PushConstantRange {
	
	// The shader stages which will have access to this push constant range.
	VkShaderStageFlags shaderStageFlags;
	
	// The size of this push constant range.
	uint32_t size;
	
} PushConstantRange;

bool validatePipeline(const Pipeline pipeline);

bool deletePipeline(Pipeline *const pPipeline);

VkPipelineLayout createPipelineLayout(VkDevice vkDevice, VkDescriptorSetLayout vkDescriptorSetLayout);

VkPipelineLayout createPipelineLayout2(const VkDevice vkDevice, 
		const uint32_t vkDescriptorSetLayoutCount, const VkDescriptorSetLayout vkDescriptorSetLayouts[static const vkDescriptorSetLayoutCount],
		const uint32_t pushConstantRangeCount, const PushConstantRange PushConstantRanges[static const pushConstantRangeCount]);

#endif	// PIPELINE_H
