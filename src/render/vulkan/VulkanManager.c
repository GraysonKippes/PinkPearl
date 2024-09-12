#include "VulkanManager.h"

#include <stdalign.h>
#include <stdlib.h>
#include <string.h>

#include <vulkan/vulkan.h>

#include "debug.h"
#include "log/Logger.h"
#include "glfw/glfw_manager.h"
#include "render/render_config.h"
#include "render/stb/image_data.h"
#include "util/Types.h"

#include "CommandBuffer.h"
#include "descriptor.h"
#include "GraphicsPipeline.h"
#include "logical_device.h"
#include "queue.h"
#include "Shader.h"
#include "texture_manager.h"
#include "vertex_input.h"
#include "compute/compute_matrices.h"
#include "compute/compute_room_texture.h"

/* -- Vulkan Module Configuration -- */

const int vkConfMaxNumQuads = VK_CONF_MAX_NUM_QUADS;

/* -- Vulkan Objects -- */

static VulkanInstance vulkan_instance = { };

static VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;

WindowSurface windowSurface = { };

PhysicalDevice physical_device = { };

MemoryTypeIndexSet memory_type_set = { };

VkDevice device = VK_NULL_HANDLE;

Swapchain swapchain = { };

Pipeline graphicsPipeline = { };

Pipeline graphicsPipelineDebug = { };

Sampler samplerDefault = { };

/* -- Queues -- */

VkQueue queueGraphics;
VkQueue queuePresent;
VkQueue queueTransfer;
VkQueue queueCompute;

/* -- Command Pools -- */

CommandPool commandPoolGraphics;
CommandPool commandPoolTransfer;
CommandPool commandPoolCompute;

/* -- Render Objects -- */

FrameArray frame_array = { };

/* -- Global buffers -- */

BufferPartition global_staging_buffer_partition;
BufferPartition global_uniform_buffer_partition;
BufferPartition global_storage_buffer_partition;

Buffer bufferDrawInfo = nullptr;

ModelPool modelPoolMain = nullptr;

ModelPool modelPoolDebug = nullptr;

/* -- Graphics Pipeline Descriptor Set Layout -- */

static const DescriptorBinding graphicsPipelineDescriptorBindings[5] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },	// Draw data
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_VERTEX_BIT },	// Matrix buffer
	{ .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, .count = VK_CONF_MAX_NUM_QUADS, .stages = VK_SHADER_STAGE_FRAGMENT_BIT },	// Texture array
	{ .type = VK_DESCRIPTOR_TYPE_SAMPLER, .count = 1, .stages = VK_SHADER_STAGE_FRAGMENT_BIT },	// Sampler
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_FRAGMENT_BIT }	// Lighting
};

static const DescriptorSetLayout graphicsPipelineDescriptorSetLayout = {
	.num_bindings = 5,
	.bindings = (DescriptorBinding *const)graphicsPipelineDescriptorBindings
};

/* -- Function Definitions -- */

static void create_global_staging_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global staging buffer...");

	const BufferPartitionCreateInfo buffer_partition_create_info = {
		.physical_device = physical_device.vkPhysicalDevice,
		.device = device,
		.buffer_type = BUFFER_TYPE_STAGING,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 3,
		.partition_sizes = (VkDeviceSize[3]){
			5120,	// Render object mesh data--vertices
			768,	// Render object mesh data--indices
			262144	// Loaded image data
		}
	};
	
	global_staging_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_uniform_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global uniform buffer...");

	const BufferPartitionCreateInfo buffer_partition_create_info = {
		.physical_device = physical_device.vkPhysicalDevice,
		.device = device,
		.buffer_type = BUFFER_TYPE_UNIFORM,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 3,
		.partition_sizes = (VkDeviceSize[3]){
			2096,	// Compute matrices
			5120,	// Compute room texture
			1812	// Lighting data
		}
	};
	
	global_uniform_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_storage_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global storage buffer...");

	const BufferPartitionCreateInfo buffer_partition_create_info = {
		.physical_device = physical_device.vkPhysicalDevice,
		.device = device,
		.buffer_type = BUFFER_TYPE_STORAGE,
		.memory_type_set = memory_type_set,
		.num_queue_family_indices = 0,
		.queue_family_indices = nullptr,
		.num_partition_sizes = 1,
		.partition_sizes = (VkDeviceSize[1]){
			4224	// Compute matrices
		}
	};
	
	global_storage_buffer_partition = create_buffer_partition(buffer_partition_create_info);
}

static void create_global_draw_data_buffer(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating global draw data buffer...");
	
	const BufferCreateInfo bufferCreateInfo = {
		.physicalDevice = physical_device,
		.vkDevice = device,
		.bufferType = BUFFER_TYPE_DRAW_DATA,
		.memoryTypeIndexSet = memory_type_set,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = nullptr,
		.subrangeCount = 2,
		.pSubrangeSizes = (VkDeviceSize[2]){
			4 + 256 * drawCommandStride,
			4 + 256 * drawCommandStride
		}
	};
	
	createBuffer(bufferCreateInfo, &bufferDrawInfo);
}

void create_vulkan_objects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Initializing Vulkan...");

	vulkan_instance = create_vulkan_instance();

	if (debug_enabled) {
		setup_debug_messenger(vulkan_instance.handle, &debug_messenger);
	}

	windowSurface = createWindowSurface(vulkan_instance);

	physical_device = select_physical_device(vulkan_instance, windowSurface);
	memory_type_set = select_memory_types(physical_device.vkPhysicalDevice);

	create_device(vulkan_instance, physical_device, &device);

	create_global_staging_buffer();
	create_global_uniform_buffer();
	create_global_storage_buffer();
	create_global_draw_data_buffer();

	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.graphics_family_ptr, 0, &queueGraphics);
	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.present_family_ptr, 0, &queuePresent);
	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.transfer_family_ptr, 0, &queueTransfer);
	vkGetDeviceQueue(device, *physical_device.queueFamilyIndices.compute_family_ptr, 0, &queueCompute);

	commandPoolGraphics = createCommandPool(device, *physical_device.queueFamilyIndices.graphics_family_ptr, false, true);
	commandPoolTransfer = createCommandPool(device, *physical_device.queueFamilyIndices.transfer_family_ptr, true, true);
	commandPoolCompute = createCommandPool(device, *physical_device.queueFamilyIndices.compute_family_ptr, true, false);

	swapchain = createSwapchain(get_application_window(), windowSurface, physical_device, device, VK_NULL_HANDLE);
	
	ShaderModule vertexShaderModule = createShaderModule(device, SHADER_STAGE_VERTEX, VERTEX_SHADER_NAME);
	ShaderModule fragmentShaderModule = createShaderModule(device, SHADER_STAGE_FRAGMENT, FRAGMENT_SHADER_NAME);
	
	GraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.vkDevice = device,
		.swapchain = swapchain,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.descriptorSetLayout = graphicsPipelineDescriptorSetLayout,
		.shaderModuleCount = 2,
		.pShaderModules = (ShaderModule[2]){ vertexShaderModule, fragmentShaderModule },
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	graphicsPipeline = createGraphicsPipeline2(graphicsPipelineCreateInfo);
	
	GraphicsPipelineCreateInfo graphicsPipelineDebugCreateInfo = {
		.vkDevice = device,
		.swapchain = swapchain,
		.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.descriptorSetLayout = graphicsPipelineDescriptorSetLayout,
		.shaderModuleCount = 2,
		.pShaderModules = (ShaderModule[2]){ vertexShaderModule, fragmentShaderModule },
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = nullptr
	};
	graphicsPipelineDebug = createGraphicsPipeline2(graphicsPipelineDebugCreateInfo);
	
	destroyShaderModule(&vertexShaderModule);
	destroyShaderModule(&fragmentShaderModule);

	samplerDefault = createSampler(device, physical_device);
	
	const FrameArrayCreateInfo frameArrayCreateInfo = {
		.num_frames = 2,
		.physical_device = physical_device,
		.vkDevice = device,
		.commandPool = commandPoolGraphics,
		.descriptorPool = (DescriptorPool){
			.handle = graphicsPipeline.vkDescriptorPool,
			.layout = graphicsPipeline.vkDescriptorSetLayout,
			.vkDevice = device
		}
	};
	frame_array = createFrameArray(frameArrayCreateInfo);
	
	init_compute_matrices(device);
	init_compute_room_texture(device);
	
	createModelPool(bufferDrawInfo, 0, 0, 4, 0, 6, 0, 256, &modelPoolMain);
	createModelPool(bufferDrawInfo, 1, 256 * 4, 4, 6, 8, 256, 256, &modelPoolDebug);
}

void destroy_vulkan_objects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Destroying Vulkan objects...");

	vkDeviceWaitIdle(device);

	terminate_compute_matrices();
	terminate_compute_room_texture();
	terminateTextureManager();

	deleteModelPool(&modelPoolDebug);
	deleteModelPool(&modelPoolMain);

	deleteFrameArray(&frame_array);
	
	deleteSampler(&samplerDefault);

	deletePipeline(&graphicsPipeline);
	deletePipeline(&graphicsPipelineDebug);
	
	deleteSwapchain(&swapchain);
	
	deleteCommandPool(&commandPoolGraphics);
	deleteCommandPool(&commandPoolTransfer);
	deleteCommandPool(&commandPoolCompute);

	deleteBuffer(&bufferDrawInfo);

	destroy_buffer_partition(&global_staging_buffer_partition);
	destroy_buffer_partition(&global_uniform_buffer_partition);
	destroy_buffer_partition(&global_storage_buffer_partition);

	vkDestroyDevice(device, nullptr);
	
	deleteWindowSurface(&windowSurface);
	
	deletePhysicalDevice(&physical_device);
	
	destroy_debug_messenger(vulkan_instance.handle, debug_messenger);
	destroy_vulkan_instance(vulkan_instance);
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Destroyed Vulkan objects.");
}

void initTextureDescriptors(void) {
	
	const Texture texture = getTexture(textureHandleMissing);
	
	VkDescriptorImageInfo descriptorImageInfos[VK_CONF_MAX_NUM_QUADS];
	for (int i = 0; i < vkConfMaxNumQuads; ++i) {
		descriptorImageInfos[i] = makeDescriptorImageInfo(texture.image);
	}
	
	VkDescriptorImageInfo descriptorSamplerInfo = makeDescriptorSamplerInfo(samplerDefault);
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		
		const VkWriteDescriptorSet descriptorWrites[2] = {{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame_array.frames[i].descriptorSet.vkDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = (uint32_t)vkConfMaxNumQuads,
			.pBufferInfo = nullptr,
			.pImageInfo = descriptorImageInfos,
			.pTexelBufferView = nullptr
		}, {
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame_array.frames[i].descriptorSet.vkDescriptorSet,
			.dstBinding = 3,
			.dstArrayElement = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
			.descriptorCount = 1,
			.pBufferInfo = nullptr,
			.pImageInfo = &descriptorSamplerInfo,
			.pTexelBufferView = nullptr
		}};
		
		vkUpdateDescriptorSets(device, 2, descriptorWrites, 0, nullptr);
	}
}

static void uploadLightingData(void) {
	
	typedef struct ambient_light_t {
		float red;
		float green;
		float blue;
		float intensity;
	} ambient_light_t;
	
	typedef struct point_light_t {
		float pos_x;
		float pos_y;
		float pos_z;
		float red;
		float green;
		float blue;
		float intensity;
	} point_light_t;
	
	ambient_light_t ambient_lighting = {
		.red = 0.125F,
		.green = 0.25F,
		.blue = 0.5F,
		.intensity = 0.825
	};
	
	const uint32_t num_point_lights = 1;
	point_light_t point_lights[NUM_RENDER_OBJECT_SLOTS];
	
	for (uint32_t i = 0; i < numRenderObjectSlots; ++i) {
		point_lights[i] = (point_light_t){
			.pos_x = 0.0F,
			.pos_y = 0.0F,
			.pos_z = 0.0F,
			.red = 0.0F,
			.green = 0.0F,
			.blue = 0.0F,
			.intensity = 0.0F
		};
	}
	
	point_lights[0] = (point_light_t){
		.pos_x = 0.0F,
		.pos_y = 0.0F,
		.pos_z = 0.5F,
		.red = 1.0F,
		.green = 0.825F,
		.blue = 0.0F,
		.intensity = 1.0F
	};
	
	byte_t *lighting_data_mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 2);
	memcpy(lighting_data_mapped_memory, &ambient_lighting, sizeof(ambient_lighting));
	memcpy(&lighting_data_mapped_memory[16], &num_point_lights, sizeof(uint32_t));
	memcpy(&lighting_data_mapped_memory[20], point_lights, numRenderObjectSlots * sizeof(point_light_t));
	buffer_partition_unmap_memory(global_uniform_buffer_partition);
}

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const ProjectionBounds projectionBounds) {

	vkWaitForFences(device, 1, &frame_array.frames[frame_array.current_frame].fence_frame_ready, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &frame_array.frames[frame_array.current_frame].fence_frame_ready);
	commandBufferReset(&frame_array.frames[frame_array.current_frame].commandBuffer);

	uint32_t imageIndex = 0;
	const VkResult result = vkAcquireNextImageKHR(device, swapchain.vkSwapchain, UINT64_MAX, frame_array.frames[frame_array.current_frame].semaphore_image_available.semaphore, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// Signal a semaphore when the entire batch in the compute queue is done being executed.
	computeMatrices(deltaTime, projectionBounds, cameraPosition, getModelTransforms(modelPoolMain));

	VkWriteDescriptorSet descriptor_writes[3] = { { } };

	const VkDescriptorBufferInfo draw_data_buffer_info = modelPoolGetBufferDescriptorInfo(modelPoolMain);
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = frame_array.frames[frame_array.current_frame].descriptorSet.vkDescriptorSet;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &draw_data_buffer_info;
	descriptor_writes[0].pImageInfo = nullptr;
	descriptor_writes[0].pTexelBufferView = nullptr;

	const VkDescriptorBufferInfo matrix_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 0);
	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = frame_array.frames[frame_array.current_frame].descriptorSet.vkDescriptorSet;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[1].pImageInfo = nullptr;
	descriptor_writes[1].pTexelBufferView = nullptr;

	const VkDescriptorBufferInfo lighting_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 2);
	descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[2].dstSet = frame_array.frames[frame_array.current_frame].descriptorSet.vkDescriptorSet;
	descriptor_writes[2].dstBinding = 4;
	descriptor_writes[2].dstArrayElement = 0;
	descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[2].descriptorCount = 1;
	descriptor_writes[2].pBufferInfo = &lighting_buffer_info;
	descriptor_writes[2].pImageInfo = nullptr;
	descriptor_writes[2].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 3, descriptor_writes, 0, nullptr);
	
	uploadLightingData();

	commandBufferBegin(&frame_array.frames[frame_array.current_frame].commandBuffer, false); {
		
		static const ImageSubresourceRange imageSubresourceRange = {
			.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseArrayLayer = 0,
			.arrayLayerCount = 1
		};
		
		const VkImageMemoryBarrier2 swapchainTransitionBarrier1 = makeImageTransitionBarrier(swapchain.pImages[imageIndex], imageSubresourceRange, imageUsageColorAttachment);
		
		const VkDependencyInfo dependencyInfo1 = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &swapchainTransitionBarrier1
		};
		
		vkCmdPipelineBarrier2(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, &dependencyInfo1);
		
		const VkRenderingAttachmentInfo attachmentInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = swapchain.pImages[imageIndex].vkImageView,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.resolveMode = VK_RESOLVE_MODE_NONE,
			.resolveImageView = VK_NULL_HANDLE,
			.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = (VkClearValue){ .color = { { 0.0F, 0.0F, 0.0F, 1.0F } } }
		};
		
		const VkRenderingInfo renderingInfo = {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.pNext = nullptr,
			.flags = 0,
			.renderArea = (VkRect2D){ { 0, 0 }, swapchain.imageExtent },
			.layerCount = 1,
			.viewMask = 0,
			.colorAttachmentCount = 1,
			.pColorAttachments = &attachmentInfo,
			.pDepthAttachment = nullptr,
			.pStencilAttachment = nullptr
		};
		
		vkCmdBeginRendering(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, &renderingInfo);
		
		// Resource binding
		
		commandBufferBindPipeline(&frame_array.frames[frame_array.current_frame].commandBuffer, graphicsPipeline);
		
		commandBufferBindDescriptorSet(&frame_array.frames[frame_array.current_frame].commandBuffer, &frame_array.frames[frame_array.current_frame].descriptorSet, graphicsPipeline);
		
		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, 0, 1, &frame_array.frames[frame_array.current_frame].vertex_buffer, offsets);
		vkCmdBindIndexBuffer(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, frame_array.frames[frame_array.current_frame].index_buffer, 0, VK_INDEX_TYPE_UINT16);
		
		// Draw call
		
		const uint32_t drawOffset = drawCountSize;
		const VkBuffer bufferDrawInfoHandle = bufferGetVkBuffer(bufferDrawInfo);
		const uint32_t maxDrawCount = modelPoolGetMaxModelCount(modelPoolMain);
		vkCmdDrawIndexedIndirectCount(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, 
				bufferDrawInfoHandle, drawOffset, 
				bufferDrawInfoHandle, 0, 
				maxDrawCount, drawCommandStride);
		
		commandBufferBindPipeline(&frame_array.frames[frame_array.current_frame].commandBuffer, graphicsPipelineDebug);
		
		const uint32_t debugDrawOffset = maxDrawCount * drawCommandStride;
		const uint32_t debugMaxDrawCount = modelPoolGetMaxModelCount(modelPoolDebug);
		vkCmdDrawIndexedIndirectCount(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, 
				bufferDrawInfoHandle, drawOffset + debugDrawOffset, 
				bufferDrawInfoHandle, debugDrawOffset, 
				debugMaxDrawCount, drawCommandStride);
		
		vkCmdEndRendering(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer);
		
		const VkImageMemoryBarrier2 swapchainTransitionBarrier2 = makeImageTransitionBarrier(swapchain.pImages[imageIndex], imageSubresourceRange, imageUsagePresent);
		
		const VkDependencyInfo dependencyInfo2 = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &swapchainTransitionBarrier2
		};
		
		vkCmdPipelineBarrier2(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, &dependencyInfo2);
	
	} commandBufferEnd(&frame_array.frames[frame_array.current_frame].commandBuffer);

	const VkCommandBufferSubmitInfo command_buffer_submit_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer,
		.deviceMask = 0
	};

	VkSemaphoreSubmitInfo wait_semaphore_submit_infos[3] = { };
	wait_semaphore_submit_infos[1] = make_timeline_semaphore_wait_submit_info(frame_array.frames[frame_array.current_frame].semaphore_buffers_ready, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT);

	wait_semaphore_submit_infos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_infos[0].pNext = nullptr;
	wait_semaphore_submit_infos[0].semaphore = frame_array.frames[frame_array.current_frame].semaphore_image_available.semaphore;
	wait_semaphore_submit_infos[0].value = 0;
	wait_semaphore_submit_infos[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	wait_semaphore_submit_infos[0].deviceIndex = 0;

	wait_semaphore_submit_infos[2].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_infos[2].pNext = nullptr;
	wait_semaphore_submit_infos[2].semaphore = compute_matrices_semaphore;
	wait_semaphore_submit_infos[2].value = 0;
	wait_semaphore_submit_infos[2].stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	wait_semaphore_submit_infos[2].deviceIndex = 0;

	VkSemaphoreSubmitInfo signal_semaphore_submit_infos[1] = { };
	signal_semaphore_submit_infos[0] = make_timeline_semaphore_signal_submit_info(frame_array.frames[frame_array.current_frame].semaphore_render_finished, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
	frame_array.frames[frame_array.current_frame].semaphore_render_finished.wait_counter += 1;

	const VkSubmitInfo2 submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.flags = 0,
		.waitSemaphoreInfoCount = 3,
		.pWaitSemaphoreInfos = wait_semaphore_submit_infos,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &command_buffer_submit_info,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = signal_semaphore_submit_infos
	};

	vkQueueSubmit2(queueGraphics, 1, &submit_info, frame_array.frames[frame_array.current_frame].fence_frame_ready);



	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = nullptr,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_array.frames[frame_array.current_frame].semaphore_present_ready.semaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.vkSwapchain,
		.pImageIndices = &imageIndex,
		.pResults = nullptr
	};

	timeline_to_binary_semaphore_signal(queueGraphics, frame_array.frames[frame_array.current_frame].semaphore_render_finished, frame_array.frames[frame_array.current_frame].semaphore_present_ready);
	vkQueuePresentKHR(queuePresent, &present_info);

	frame_array.current_frame = (frame_array.current_frame + 1) % frame_array.num_frames;
}