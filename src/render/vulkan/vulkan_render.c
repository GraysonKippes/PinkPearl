#include "vulkan_render.h"

#include <stdlib.h>
#include <string.h>

#include <DataStuff/CmpFunc.h>
#include <DataStuff/BinarySearchTree.h>
#include <DataStuff/LinkedList.h>
#include <DataStuff/Stack.h>

#include "log/Logger.h"
#include "game/area/room.h"
#include "render/render_config.h"
#include "util/time.h"

#include "CommandBuffer.h"
#include "texture.h"
#include "texture_manager.h"
#include "TextureState.h"
#include "vertex_input.h"
#include "vulkan_manager.h"
#include "compute/compute_matrices.h"
#include "compute/compute_room_texture.h"

/* -- Draw Data -- */

typedef struct DrawData {
	// Indirect draw command data
	uint32_t indexCount;
	uint32_t instanceCount;
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;
	// Additional draw info
	int32_t quadHandle;
	uint32_t imageIndex;
} DrawData;

// Array of draw datas which are uploaded to the GPU.
static uint32_t drawDataCount = 0;
static DrawData drawDatas[VK_CONF_MAX_NUM_QUADS];

// Stores IDs for unused quads.
Stack inactiveQuadIDs;

/* -- Quad object fields -- */

// Maps a quad ID to a draw data in the GPU.
uint32_t quadDrawDataIndices[VK_CONF_MAX_NUM_QUADS];

// Transform of each quad.
RenderTransform quadTransforms[VK_CONF_MAX_NUM_QUADS];

// Texture state of each quad.
TextureState quadTextureStates[VK_CONF_MAX_NUM_QUADS];

/* -- Function Declarations -- */

static bool uploadQuadMesh(const int quadHandle, const BoxF quadDimensions);

static void insertDrawData(const int quadHandle, const float quadDepth, const unsigned int imageIndex);

static void removeDrawData(const int quadHandle);

static void updateTextureDescriptor(const int quadHandle, const int textureHandle);

void createVulkanRenderObjects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating Vulkan render objects...");

	init_compute_matrices(device);
	init_compute_room_texture(device);
	
	inactiveQuadIDs = newStack(vkConfMaxNumQuads, sizeof(int));
	for (int quadHandle = vkConfMaxNumQuads - 1; quadHandle >= 0; --quadHandle) {
		stackPush(&inactiveQuadIDs, &quadHandle);
	}
}

void destroyVulkanRenderObjects(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Destroying Vulkan render objects...");
	
	vkDeviceWaitIdle(device);

	terminate_compute_matrices();
	terminate_compute_room_texture();
	terminateTextureManager();
	
	deleteStack(&inactiveQuadIDs);
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

int loadQuad(const BoxF quadDimensions, const Vector4F quadPosition, const TextureState quadTextureState) {
	if (stackIsEmpty(inactiveQuadIDs)) {
		return -1;
	}
	
	// Retrieve first quad ID and push draw info.
	int quadHandle = -1;
	stackPop(&inactiveQuadIDs, &quadHandle);
	if (!validateQuadHandle(quadHandle)) {
		return -1;
	}
	
	setQuadTranslation(quadHandle, quadPosition);
	setQuadTranslation(quadHandle, quadPosition);
	setQuadScaling(quadHandle, zeroVector4F);
	setQuadScaling(quadHandle, zeroVector4F);
	setQuadRotation(quadHandle, zeroVector4F);
	setQuadRotation(quadHandle, zeroVector4F);
	quadTextureStates[quadHandle] = quadTextureState;
	
	uploadQuadMesh(quadHandle, quadDimensions);
	insertDrawData(quadHandle, quadPosition.z, 0);
	updateTextureDescriptor(quadHandle, quadTextureState.textureHandle);
	
	return quadHandle;
}

void unloadQuad(int *const pQuadHandle) {
	if (!pQuadHandle) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error unloading quad: pointer to quad ID is null.");
		return;
	} else if (!validateQuadHandle(*pQuadHandle)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error unloading quad: quad ID (%i) is invalid.", *pQuadHandle);
		return;
	}
	
	removeDrawData(*pQuadHandle);
	stackPush(&inactiveQuadIDs, pQuadHandle);
	*pQuadHandle = -1;
}

bool validateQuadHandle(const int quadHandle) {
	return quadHandle >= 0 && quadHandle < vkConfMaxNumQuads;
}

bool setQuadTranslation(const int quadHandle, const Vector4F translation) {
	if (!validateQuadHandle(quadHandle)) {
		return false;
	}
	renderVectorSet(&quadTransforms[quadHandle].translation, translation);
	return true;
}

bool setQuadScaling(const int quadHandle, const Vector4F scaling) {
	if (!validateQuadHandle(quadHandle)) {
		return false;
	}
	renderVectorSet(&quadTransforms[quadHandle].scaling, scaling);
	return true;
}

bool setQuadRotation(const int quadHandle, const Vector4F rotation) {
	if (!validateQuadHandle(quadHandle)) {
		return false;
	}
	renderVectorSet(&quadTransforms[quadHandle].rotation, rotation);
	return true;
}

TextureState *getQuadTextureState(const int quadHandle) {
	if (!validateQuadHandle(quadHandle)) {
		return nullptr;
	}
	return &quadTextureStates[quadHandle];
}

static bool uploadQuadMesh(const int quadHandle, const BoxF quadDimensions) {
	if (!validateQuadHandle(quadHandle)) {
		return false;
	}
	
	const float vertices[NUM_VERTICES_PER_QUAD * VERTEX_INPUT_ELEMENT_STRIDE] = {
		// Positions									Texture			Color
		quadDimensions.x1, quadDimensions.y1, 0.0F,		0.0F, 1.0F,		1.0F, 1.0F, 1.0F,	// 0: Bottom-left
		quadDimensions.x1, quadDimensions.y2, 0.0F,		0.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// 1: Top-left
		quadDimensions.x2, quadDimensions.y2, 0.0F,		1.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// 2: Top-right
		quadDimensions.x2, quadDimensions.y1, 0.0F,		1.0F, 1.0F,		1.0F, 1.0F, 1.0F	// 3: Bottom-right
	};
	
	const VkDeviceSize verticesSize = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize verticesOffset = verticesSize * (VkDeviceSize)quadHandle;
	
	byte_t *const mappedMemoryVertices = buffer_partition_map_memory(global_staging_buffer_partition, 0);
	memcpy(&mappedMemoryVertices[verticesOffset], vertices, verticesSize);
	buffer_partition_unmap_memory(global_staging_buffer_partition);
	
	// Transfer vertex data to frame vertex buffers.
	
	static VkCommandBuffer cmdBufs[NUM_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };
	
	VkSemaphore waitSemaphores[NUM_FRAMES_IN_FLIGHT];
	uint64_t waitSemaphoreValues[NUM_FRAMES_IN_FLIGHT];
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		waitSemaphores[i] = frame_array.frames[i].semaphore_buffers_ready.semaphore;
		waitSemaphoreValues[i] = frame_array.frames[i].semaphore_buffers_ready.wait_counter;
	}
	
	const VkSemaphoreWaitInfo semaphoreWaitInfo = {
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
		.pNext = nullptr,
		.flags = 0,
		.semaphoreCount = frame_array.num_frames,
		.pSemaphores = waitSemaphores,
		.pValues = waitSemaphoreValues
	};
	vkWaitSemaphores(device, &semaphoreWaitInfo, UINT64_MAX);
	
	vkFreeCommandBuffers(device, commandPoolTransfer.vkCommandPool, frame_array.num_frames, cmdBufs);
	allocCmdBufs(device, commandPoolTransfer.vkCommandPool, frame_array.num_frames, cmdBufs);
	
	VkCommandBufferSubmitInfo cmdBufSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphoreWaitSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphoreSignalSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSubmitInfo2 submitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	
	const VkBufferCopy bufferCopy = {
		.srcOffset = verticesOffset,
		.dstOffset = verticesOffset,
		.size = verticesSize
	};
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		cmdBufBegin(cmdBufs[i], true);
		vkCmdCopyBuffer(cmdBufs[i], global_staging_buffer_partition.buffer, frame_array.frames[i].vertex_buffer, 1, &bufferCopy);
		vkEndCommandBuffer(cmdBufs[i]);
		
		cmdBufSubmitInfos[i] = make_command_buffer_submit_info(cmdBufs[i]);
		semaphoreWaitSubmitInfos[i] = make_timeline_semaphore_wait_submit_info(frame_array.frames[i].semaphore_render_finished, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		semaphoreSignalSubmitInfos[i] = make_timeline_semaphore_signal_submit_info(frame_array.frames[i].semaphore_buffers_ready, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		frame_array.frames[i].semaphore_buffers_ready.wait_counter += 1;

		submitInfos[i] = (VkSubmitInfo2){
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = nullptr,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &semaphoreWaitSubmitInfos[i],
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdBufSubmitInfos[i],
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semaphoreSignalSubmitInfos[i]
		};
	}
	vkQueueSubmit2(queueTransfer, frame_array.num_frames, submitInfos, VK_NULL_HANDLE);
	
	return true;
}

static DrawData makeDrawData(const int quadHandle, const unsigned int imageIndex) {
	return (DrawData){
		.indexCount = num_indices_per_rect,
		.instanceCount = 1,
		.firstIndex = 0,
		.vertexOffset = num_vertices_per_rect * quadHandle,
		.firstInstance = 0,
		.quadHandle = quadHandle,
		.imageIndex = imageIndex
	};
}

static void insertDrawData(const int quadHandle, const float quadDepth, const unsigned int imageIndex) {
	if (vkConfMaxNumQuads - (int)drawDataCount <= 0) {
		return;
	}
	
	// Iterate over the already loaded quads to compare depth and determine where to insert this quad.
	uint32_t insertIndex = drawDataCount;
	for (uint32_t i = 0; i < drawDataCount; ++i) {
		const float otherDepth = quadTransforms[drawDatas[i].quadHandle].translation.current.z;
		if (quadDepth > otherDepth) {
			insertIndex = i;
			break;
		}
	}
	
	// Move each draw data after the inserted draw data forward one place.
	for (uint32_t i = drawDataCount; i > insertIndex; --i) {
		drawDatas[i] = drawDatas[i - 1];
		quadDrawDataIndices[drawDatas[i].quadHandle] = i;
	}
	
	// Insert the new draw data.
	drawDatas[insertIndex] = makeDrawData(quadHandle, imageIndex);
	quadDrawDataIndices[quadHandle] = insertIndex;
	drawDataCount += 1;
	
	// TODO - cut the partition crap and just do one mapped memory.
	uint8_t *const draw_count_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 0);
	memcpy(draw_count_mapped_memory, &drawDataCount, sizeof(drawDataCount));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	uint8_t *const draw_data_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(draw_data_mapped_memory, drawDatas, drawDataCount * sizeof(drawDatas[0]));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
}

static void removeDrawData(const int quadHandle) {
	
	// Get the index of the draw data associated with the quad.
	const uint32_t drawDataIndex = quadDrawDataIndices[quadHandle];
	if (drawDataIndex >= drawDataCount) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error removing draw data: quad's draw data index (%u) is not less than draw data count (%u) (quad ID = %i).", drawDataIndex, drawDataCount, quadHandle);
		return;
	}
	
	// Remove the draw data associated with the quad.
	drawDataCount -= 1;
	// Move each draw data ahead of the removed draw data forward one place.
	for (uint32_t i = drawDataIndex; i < drawDataCount; ++i) {
		// i = index of the draw data after being moved forward one place.
		// i + 1 = index of the draw data before being moved forward one place.
		drawDatas[i] = drawDatas[i + 1];	// Copy the next draw data into this place.
		quadDrawDataIndices[drawDatas[i].quadHandle] = i;
	}
	
	// TODO - cut the partition crap and just do one mapped memory.
	uint8_t *const draw_count_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 0);
	memcpy(draw_count_mapped_memory, &drawDataCount, sizeof(drawDataCount));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	uint8_t *const draw_data_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(draw_data_mapped_memory, drawDatas, drawDataCount * sizeof(drawDatas[0]));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
}

void updateDrawData(const int quadHandle, const unsigned int imageIndex) {
	if (!validateQuadHandle(quadHandle)) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error updating draw data: quad ID (%i) is invalid.", quadHandle);
		return;
	}
	
	const uint32_t drawDataIndex = quadDrawDataIndices[quadHandle];
	drawDatas[drawDataIndex] = makeDrawData(quadHandle, imageIndex);
	
	uint8_t *const drawDataMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(&drawDataMappedMemory[drawDataIndex * sizeof(DrawData)], &drawDatas[drawDataIndex], sizeof(DrawData));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
}

static void updateTextureDescriptor(const int quadHandle, const int textureHandle) {
	
	const Texture texture = getTexture(textureHandle);
	VkWriteDescriptorSet writeDescriptorSets[NUM_FRAMES_IN_FLIGHT] = { { } };
	
	// Replace malloc with a better suited allocation, perhaps an arena?
	VkDescriptorImageInfo *pDescriptorImageInfo = malloc(sizeof(VkDescriptorImageInfo));
	*pDescriptorImageInfo = makeDescriptorImageInfo(texture.image);
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		writeDescriptorSets[i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame_array.frames[i].descriptorSet.vkDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = (uint32_t)quadHandle,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
			.pBufferInfo = nullptr,
			.pImageInfo = pDescriptorImageInfo,
			.pTexelBufferView = nullptr
		};
		descriptorSetPushWrite(&frame_array.frames[i].descriptorSet, writeDescriptorSets[i]);
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
	
	byte_t *lighting_data_mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 3);
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
	computeMatrices(deltaTime, projectionBounds, cameraPosition, quadTransforms);

	VkWriteDescriptorSet descriptor_writes[3] = { { } };

	const VkDescriptorBufferInfo draw_data_buffer_info = buffer_partition_descriptor_info(global_draw_data_buffer_partition, 1);
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

	const VkDescriptorBufferInfo lighting_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 3);
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
		
		commandBufferBindPipeline(&frame_array.frames[frame_array.current_frame].commandBuffer, graphicsPipeline);
		
		commandBufferBindDescriptorSet(&frame_array.frames[frame_array.current_frame].commandBuffer, &frame_array.frames[frame_array.current_frame].descriptorSet, graphicsPipeline);
		
		const VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, 0, 1, &frame_array.frames[frame_array.current_frame].vertex_buffer, offsets);
		vkCmdBindIndexBuffer(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, frame_array.frames[frame_array.current_frame].index_buffer, 0, VK_INDEX_TYPE_UINT16);
		
		const uint32_t draw_data_offset = global_draw_data_buffer_partition.ranges[1].offset;
		vkCmdDrawIndexedIndirectCount(frame_array.frames[frame_array.current_frame].commandBuffer.vkCommandBuffer, global_draw_data_buffer_partition.buffer, draw_data_offset, global_draw_data_buffer_partition.buffer, 0, 68, 28);
		
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
