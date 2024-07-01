#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>

typedef enum PipelineType {
	PIPELINE_TYPE_GRAPHICS,
	PIPELINE_TYPE_COMPUTE
} PipelineType;

typedef struct Pipeline {

	VkPipeline vkPipeline;
	VkPipelineLayout vkPipelineLayout;
	VkDevice vkDevice;
	
	PipelineType type;

} Pipeline;

#endif	// PIPELINE_H
