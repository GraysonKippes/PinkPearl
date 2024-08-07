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

#endif	// PIPELINE_H
