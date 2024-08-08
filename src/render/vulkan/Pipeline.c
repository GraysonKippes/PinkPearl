#include "Pipeline.h"

bool destroyPipeline(Pipeline *const pPipeline) {
	if (!pPipeline) {
		return false;
	}
	
	// Destroy the Vulkan objects associated with the pipeline struct.
	vkDestroyPipeline(pPipeline->vkDevice, pPipeline->vkPipeline, nullptr);
	vkDestroyPipelineLayout(pPipeline->vkDevice, pPipeline->vkPipelineLayout, nullptr);
	vkDestroyDescriptorPool(pPipeline->vkDevice, pPipeline->vkDescriptorPool, nullptr);
	vkDestroyDescriptorSetLayout(pPipeline->vkDevice, pPipeline->vkDescriptorSetLayout, nullptr);
	
	// Nullify the handles inside the struct.
	pPipeline->vkPipeline = VK_NULL_HANDLE;
	pPipeline->vkPipelineLayout = VK_NULL_HANDLE;
	pPipeline->vkDescriptorPool = VK_NULL_HANDLE;
	pPipeline->vkDescriptorSetLayout = VK_NULL_HANDLE;
	pPipeline->vkDevice = VK_NULL_HANDLE;

	return true;
}