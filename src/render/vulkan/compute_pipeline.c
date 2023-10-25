#include "compute_pipeline.h"

#include <stdarg.h>
#include <stdlib.h>

#include "log/logging.h"

void create_pipeline_layout(VkDevice logical_device, VkDescriptorSetLayout descriptor_set_layout, VkPipelineLayout *pipeline_layout_ptr) {

	VkPipelineLayoutCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.setLayoutCount = 1;
	create_info.pSetLayouts = &descriptor_set_layout;
	create_info.pushConstantRangeCount = 0;
	create_info.pPushConstantRanges = NULL;

	vkCreatePipelineLayout(logical_device, &create_info, NULL, pipeline_layout_ptr);
}

void create_compute_pipelines(VkDevice logical_device, VkPipeline *compute_pipelines, VkPipelineLayout *pipeline_layouts, uint32_t num_compute_shaders, ...) {

	log_message(VERBOSE, "Creating compute pipelines...");

	va_list compute_shaders;
	va_start(compute_shaders, num_compute_shaders);

	VkComputePipelineCreateInfo *create_infos = calloc(num_compute_shaders, sizeof(VkComputePipelineCreateInfo));

	for (uint32_t i = 0; i < num_compute_shaders; ++i) {

		compute_shader_t compute_shader = va_arg(compute_shaders, compute_shader_t);

		create_pipeline_layout(logical_device, compute_shader.m_descriptor_set_layout, (pipeline_layouts + i));

		create_infos[i].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		create_infos[i].pNext = NULL;
		create_infos[i].flags = 0;
		create_infos[i].layout = pipeline_layouts[i];
		create_infos[i].stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		create_infos[i].stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		create_infos[i].stage.module = compute_shader.m_module;
		create_infos[i].stage.pName = "main";
	}

	VkResult result = vkCreateComputePipelines(logical_device, VK_NULL_HANDLE, num_compute_shaders, create_infos, NULL, compute_pipelines);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Compute pipeline creation failed. (Error code: %i)", result);
	}

	free(create_infos);
	va_end(compute_shaders);
}
