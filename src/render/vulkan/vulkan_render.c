#include "vulkan_render.h"

#include <stdlib.h>
#include <string.h>

#include <DataStuff/CmpFunc.h>
#include <DataStuff/BinarySearchTree.h>
#include <DataStuff/LinkedList.h>
#include <DataStuff/Stack.h>

#include "log/log_stack.h"
#include "log/logging.h"
#include "game/area/room.h"
#include "render/render_config.h"
#include "util/time.h"

#include "command_buffer.h"
#include "texture.h"
#include "texture_manager.h"
#include "vertex_input.h"
#include "vulkan_manager.h"
#include "compute/compute_matrices.h"
#include "compute/compute_room_texture.h"

typedef struct DrawData {
	// Indirect draw info
	uint32_t index_count;
	uint32_t instance_count;
	uint32_t first_index;
	int32_t vertex_offset;
	uint32_t first_instance;
	// Additional draw info
	int32_t quadID;
	uint32_t image_index;
} DrawData;

typedef struct DrawInfo {
	int quadID;
	uint32_t drawDataIndex;	// The index in array of draw datas.
} DrawInfo;

// Stores draw infos, including IDs for quads currently being rendered.
BinarySearchTree activeQuadIDs;

// Stores IDs for unused quads.
Stack inactiveQuadIDs;

static int cmpFuncDrawInfo(const void *const pA, const void *const pB) {
	if (pA == NULL || pB == NULL) {
		return -2;
	}
	
	const DrawInfo drawInfo_a = *(DrawInfo *)pA;
	const DrawInfo drawInfo_b = *(DrawInfo *)pB;
	if (drawInfo_a.quadID < drawInfo_b.quadID) {
		return -1;
	}
	else if (drawInfo_a.quadID > drawInfo_b.quadID) {
		return 1;
	}
	return 0;
}

static bool uploadQuadMesh(const int quadID, const DimensionsF quadDimensions);
static void rebuildDrawData(void);
static void updateTextureDescriptor(const int quadID, const int textureHandle);

void create_vulkan_render_objects(void) {
	log_stack_push("Vulkan");
	log_message(VERBOSE, "Creating Vulkan render objects...");

	init_compute_matrices(device);
	init_compute_room_texture(device);
	
	// Initialize data structures for managing quad IDs.
	activeQuadIDs = newBinarySearchTree(sizeof(DrawInfo), cmpFuncDrawInfo);
	inactiveQuadIDs = newStack(num_render_object_slots, sizeof(int));
	for (int quadID = (int)num_render_object_slots - 1; quadID >= 0; --quadID) {
		stackPush(&inactiveQuadIDs, &quadID);
	}
	
	log_stack_pop();
}

void destroy_vulkan_render_objects(void) {
	log_stack_push("Vulkan");
	log_message(VERBOSE, "Destroying Vulkan render objects...");
	
	vkDeviceWaitIdle(device);

	terminate_compute_matrices();
	terminate_compute_room_texture();
	destroy_textures();
	
	deleteBinarySearchTree(&activeQuadIDs);
	deleteStack(&inactiveQuadIDs);
	
	log_stack_pop();
}

int loadQuad(const DimensionsF quadDimensions, const int textureHandle) {
	int quadID = -1;
	if (stackIsEmpty(inactiveQuadIDs)) {
		return quadID;
	}
	
	// Retrieve first quad ID and push draw info.
	stackPop(&inactiveQuadIDs, &quadID);
	const DrawInfo drawInfo = {
		.quadID = quadID,
		.drawDataIndex = 0
	};
	bstInsert(&activeQuadIDs, &drawInfo);
	
	uploadQuadMesh(quadID, quadDimensions);
	updateTextureDescriptor(quadID, textureHandle);
	rebuildDrawData();
	
	return quadID;
}

void unloadQuad(const int quadID) {
	if (!bstRemove(&activeQuadIDs, &quadID)) {
		return;
	}
	stackPush(&inactiveQuadIDs, &quadID);
	rebuildDrawData();
}

bool validateQuadID(const int quadID) {
	return quadID >= 0 && quadID < (int)num_render_object_slots;
}

static bool uploadQuadMesh(const int quadID, const DimensionsF quadDimensions) {
	if (!validateQuadID(quadID)) {
		return false;
	}
	
	const float vertices[NUM_VERTICES_PER_QUAD * VERTEX_INPUT_ELEMENT_STRIDE] = {
		// Positions									Texture			Color
		quadDimensions.x1, quadDimensions.y2, 0.0F,		0.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// Top-left
		quadDimensions.x2, quadDimensions.y2, 0.0F,		1.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// Top-right
		quadDimensions.x2, quadDimensions.y1, 0.0F,		1.0F, 1.0F,		1.0F, 1.0F, 1.0F,	// Bottom-right
		quadDimensions.x1, quadDimensions.y1, 0.0F,		0.0F, 1.0F,		1.0F, 1.0F, 1.0F	// Bottom-left
	};
	
	const VkDeviceSize verticesSize = num_vertices_per_rect * vertex_input_element_stride * sizeof(float);
	const VkDeviceSize verticesOffset = verticesSize * (VkDeviceSize)quadID;
	
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
		.pNext = NULL,
		.flags = 0,
		.semaphoreCount = frame_array.num_frames,
		.pSemaphores = waitSemaphores,
		.pValues = waitSemaphoreValues
	};
	vkWaitSemaphores(device, &semaphoreWaitInfo, UINT64_MAX);
	
	vkFreeCommandBuffers(device, transfer_command_pool, frame_array.num_frames, cmdBufs);
	allocate_command_buffers(device, transfer_command_pool, frame_array.num_frames, cmdBufs);
	
	VkCommandBufferSubmitInfo cmdBufSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphoreWaitSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSemaphoreSubmitInfo semaphoreSignalSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	VkSubmitInfo2 submitInfos[NUM_FRAMES_IN_FLIGHT] = { { 0 } };
	
	const VkCommandBufferBeginInfo beginInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};
	
	const VkBufferCopy bufferCopy = {
		.srcOffset = verticesOffset,
		.dstOffset = verticesOffset,
		.size = verticesSize
	};
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		vkBeginCommandBuffer(cmdBufs[i], &beginInfo);
		vkCmdCopyBuffer(cmdBufs[i], global_staging_buffer_partition.buffer, frame_array.frames[i].vertex_buffer, 1, &bufferCopy);
		vkEndCommandBuffer(cmdBufs[i]);
		
		cmdBufSubmitInfos[i] = make_command_buffer_submit_info(cmdBufs[i]);
		semaphoreWaitSubmitInfos[i] = make_timeline_semaphore_wait_submit_info(frame_array.frames[i].semaphore_render_finished, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		semaphoreSignalSubmitInfos[i] = make_timeline_semaphore_signal_submit_info(frame_array.frames[i].semaphore_buffers_ready, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
		frame_array.frames[i].semaphore_buffers_ready.wait_counter += 1;

		submitInfos[i] = (VkSubmitInfo2){
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
			.pNext = NULL,
			.waitSemaphoreInfoCount = 1,
			.pWaitSemaphoreInfos = &semaphoreWaitSubmitInfos[i],
			.commandBufferInfoCount = 1,
			.pCommandBufferInfos = &cmdBufSubmitInfos[i],
			.signalSemaphoreInfoCount = 1,
			.pSignalSemaphoreInfos = &semaphoreSignalSubmitInfos[i]
		};
	}
	vkQueueSubmit2(transfer_queue, frame_array.num_frames, submitInfos, VK_NULL_HANDLE);
	
	return true;
}

static DrawData makeDrawData(const int quadID, const unsigned int imageIndex) {
	return (DrawData){
		.index_count = num_indices_per_rect,
		.instance_count = 1,
		.first_index = 0,
		.vertex_offset = num_vertices_per_rect * quadID,
		.first_instance = 0,
		.quadID = quadID,
		.image_index = imageIndex
	};
}

static void rebuildDrawData(void) {
	
	/* Needs to be called in the following situations:
	 * 	Texture is animated.
	 * 	New render object is created/used.
	 *	Any render object is destroyed/released.
	 *	Area render enters/exits scroll state.
	*/
	
	uint32_t draw_count = 0;
	//const uint32_t max_draw_count = NUM_RENDER_OBJECT_SLOTS + NUM_ROOM_TEXTURE_CACHE_SLOTS * NUM_ROOM_LAYERS;
	DrawData draw_datas[NUM_RENDER_OBJECT_SLOTS + NUM_ROOM_TEXTURE_CACHE_SLOTS * NUM_ROOM_LAYERS];
	
	LinkedList traversal = bstTraverseInOrder(activeQuadIDs);
	if (!linkedListValidate(traversal)) {
		logf_message(ERROR, "Error building draw data: failed to traverse active quad IDs.");
		return;
	}
	
	for (LinkedListNode *pNode = traversal.pHeadNode; pNode != NULL; pNode = pNode->pNextNode) {
		DrawInfo *const pDrawInfo = (DrawInfo *)((BinarySearchTreeNode *)pNode->pObject)->pObject;
		if (pDrawInfo == NULL) {
			deleteLinkedList(&traversal);
			return;
		}
		draw_datas[draw_count] = makeDrawData(pDrawInfo->quadID, 0);
		pDrawInfo->drawDataIndex = draw_count;
		draw_count += 1;
	}
	
	deleteLinkedList(&traversal);
	
	byte_t *draw_count_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 0);
	memcpy(draw_count_mapped_memory, &draw_count, sizeof(draw_count));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	byte_t *draw_data_mapped_memory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(draw_data_mapped_memory, draw_datas, draw_count * sizeof(DrawData));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
}

static void updateTextureDescriptor(const int quadID, const int textureHandle) {
	
	const Texture texture = getTexture(textureHandle);
	
	const VkDescriptorImageInfo descriptorImageInfo = {
		.sampler = imageSamplerDefault,
		.imageView = texture.image.vkImageView,
		.imageLayout = texture.layout
	};
	
	const VkWriteDescriptorSet descriptorWrite = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		.dstSet = frame_array.frames[frame_array.current_frame].descriptor_set,
		.dstBinding = 2,
		.dstArrayElement = (uint32_t)quadID,
		.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		.descriptorCount = 1,
		.pBufferInfo = NULL,
		.pImageInfo = &descriptorImageInfo,
		.pTexelBufferView = NULL
	};
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
	}
}

void updateDrawData(const int quadID, const unsigned int imageIndex) {
	
	// Search for the draw info in the active quad IDs tree.
	const DrawInfo searchDrawInfo = {
		.quadID = quadID,
		.drawDataIndex = 1
	};
	BinarySearchTreeNode *pSearchNode = NULL;
	bstSearch(&activeQuadIDs, &searchDrawInfo, &pSearchNode);
	if (pSearchNode == NULL) {
		return;
	} else if (pSearchNode->pObject == NULL) {
		return;
	}
	
	const DrawInfo drawInfo = *(DrawInfo *)pSearchNode->pObject;
	const DrawData drawData = makeDrawData(drawInfo.quadID, imageIndex);
	
	byte_t *drawDataMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(drawDataMappedMemory + drawInfo.drawDataIndex * sizeof(DrawData), &drawData, sizeof drawData);
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
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
	
	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
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
	memcpy(&lighting_data_mapped_memory[20], point_lights, num_render_object_slots * sizeof(point_light_t));
	buffer_partition_unmap_memory(global_uniform_buffer_partition);
}

void drawFrame(const float deltaTime, const Vector4F cameraPosition, const projection_bounds_t projectionBounds, const RenderTransform *const renderObjTransforms) {

	vkWaitForFences(device, 1, &frame_array.frames[frame_array.current_frame].fence_frame_ready, VK_TRUE, UINT64_MAX);
	vkResetFences(device, 1, &frame_array.frames[frame_array.current_frame].fence_frame_ready);
	vkResetCommandBuffer(frame_array.frames[frame_array.current_frame].command_buffer, 0);

	uint32_t image_index = 0;
	const VkResult result = vkAcquireNextImageKHR(device, swapchain.handle, UINT64_MAX, frame_array.frames[frame_array.current_frame].semaphore_image_available.semaphore, VK_NULL_HANDLE, &image_index);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) {
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		// TODO - error handling
	}

	// Signal a semaphore when the entire batch in the compute queue is done being executed.
	computeMatrices(deltaTime, projectionBounds, cameraPosition, renderObjTransforms);

	VkWriteDescriptorSet descriptor_writes[4] = { { 0 } };

	const VkDescriptorBufferInfo draw_data_buffer_info = buffer_partition_descriptor_info(global_draw_data_buffer_partition, 1);
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = frame_array.frames[frame_array.current_frame].descriptor_set;
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &draw_data_buffer_info;
	descriptor_writes[0].pImageInfo = NULL;
	descriptor_writes[0].pTexelBufferView = NULL;

	const VkDescriptorBufferInfo matrix_buffer_info = buffer_partition_descriptor_info(global_storage_buffer_partition, 0);
	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = frame_array.frames[frame_array.current_frame].descriptor_set;
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pBufferInfo = &matrix_buffer_info;
	descriptor_writes[1].pImageInfo = NULL;
	descriptor_writes[1].pTexelBufferView = NULL;

	VkDescriptorImageInfo texture_infos[NUM_RENDER_OBJECT_SLOTS + NUM_ROOM_SIZES];

	for (uint32_t i = 0; i < num_render_object_slots; ++i) {
		Texture texture = getTexture(missing_texture_handle);
		/*if (is_render_object_slot_enabled(i)) {
			const TextureState texture_state = render_object_texture_states[i];
			texture = get_loaded_texture(texture_state.handle);
		}
		else {
			texture = get_loaded_texture(missing_texture_handle);
		} */
		texture_infos[i].sampler = imageSamplerDefault;
		texture_infos[i].imageView = texture.image.vkImageView;
		texture_infos[i].imageLayout = texture.layout;
	}
	
	for (uint32_t i = 0; i < num_room_sizes; ++i) {
		const Texture room_texture = getTexture(1 + i);
		const uint32_t index = num_render_object_slots + i;
		texture_infos[index].sampler = imageSamplerDefault;
		texture_infos[index].imageView = room_texture.image.vkImageView;
		texture_infos[index].imageLayout = room_texture.layout;
	}

	descriptor_writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[2].dstSet = frame_array.frames[frame_array.current_frame].descriptor_set;
	descriptor_writes[2].dstBinding = 2;
	descriptor_writes[2].dstArrayElement = 0;
	descriptor_writes[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[2].descriptorCount = num_render_object_slots + num_room_sizes;
	descriptor_writes[2].pBufferInfo = NULL;
	descriptor_writes[2].pImageInfo = texture_infos;
	descriptor_writes[2].pTexelBufferView = NULL;

	const VkDescriptorBufferInfo lighting_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 3);
	descriptor_writes[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[3].dstSet = frame_array.frames[frame_array.current_frame].descriptor_set;
	descriptor_writes[3].dstBinding = 3;
	descriptor_writes[3].dstArrayElement = 0;
	descriptor_writes[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[3].descriptorCount = 1;
	descriptor_writes[3].pBufferInfo = &lighting_buffer_info;
	descriptor_writes[3].pImageInfo = NULL;
	descriptor_writes[3].pTexelBufferView = NULL;

	vkUpdateDescriptorSets(device, 4, descriptor_writes, 0, NULL);
	
	uploadLightingData();

	const VkCommandBufferBeginInfo begin_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = 0,
		.pInheritanceInfo = NULL
	};
	vkBeginCommandBuffer(frame_array.frames[frame_array.current_frame].command_buffer, &begin_info);

	static const VkClearValue clear_value = { { { 0 } } };
	const VkRenderPassBeginInfo render_pass_info = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = graphics_pipeline.render_pass,
		.framebuffer = swapchain.framebuffers[image_index],
		.renderArea.offset.x = 0,
		.renderArea.offset.y = 0,
		.renderArea.extent = swapchain.extent,
		.clearValueCount = 1,
		.pClearValues = &clear_value
	};
	vkCmdBeginRenderPass(frame_array.frames[frame_array.current_frame].command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
	
	vkCmdBindPipeline(frame_array.frames[frame_array.current_frame].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.handle);
	vkCmdBindDescriptorSets(frame_array.frames[frame_array.current_frame].command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline.layout, 0, 1, &frame_array.frames[frame_array.current_frame].descriptor_set, 0, NULL);
	
	const VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(frame_array.frames[frame_array.current_frame].command_buffer, 0, 1, &frame_array.frames[frame_array.current_frame].vertex_buffer, offsets);
	vkCmdBindIndexBuffer(frame_array.frames[frame_array.current_frame].command_buffer, frame_array.frames[frame_array.current_frame].index_buffer, 0, VK_INDEX_TYPE_UINT16);
	
	const uint32_t draw_data_offset = global_draw_data_buffer_partition.ranges[1].offset;
	vkCmdDrawIndexedIndirectCount(frame_array.frames[frame_array.current_frame].command_buffer, global_draw_data_buffer_partition.buffer, draw_data_offset, global_draw_data_buffer_partition.buffer, 0, 68, 28);
	
	vkCmdEndRenderPass(frame_array.frames[frame_array.current_frame].command_buffer);
	vkEndCommandBuffer(frame_array.frames[frame_array.current_frame].command_buffer);

	const VkCommandBufferSubmitInfo command_buffer_submit_info = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = NULL,
		.commandBuffer = frame_array.frames[frame_array.current_frame].command_buffer,
		.deviceMask = 0
	};

	VkSemaphoreSubmitInfo wait_semaphore_submit_infos[3] = { 0 };
	wait_semaphore_submit_infos[1] = make_timeline_semaphore_wait_submit_info(frame_array.frames[frame_array.current_frame].semaphore_buffers_ready, VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT);

	wait_semaphore_submit_infos[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_infos[0].pNext = NULL;
	wait_semaphore_submit_infos[0].semaphore = frame_array.frames[frame_array.current_frame].semaphore_image_available.semaphore;
	wait_semaphore_submit_infos[0].value = 0;
	wait_semaphore_submit_infos[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
	wait_semaphore_submit_infos[0].deviceIndex = 0;

	wait_semaphore_submit_infos[2].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
	wait_semaphore_submit_infos[2].pNext = NULL;
	wait_semaphore_submit_infos[2].semaphore = compute_matrices_semaphore;
	wait_semaphore_submit_infos[2].value = 0;
	wait_semaphore_submit_infos[2].stageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
	wait_semaphore_submit_infos[2].deviceIndex = 0;

	VkSemaphoreSubmitInfo signal_semaphore_submit_infos[1] = { 0 };
	signal_semaphore_submit_infos[0] = make_timeline_semaphore_signal_submit_info(frame_array.frames[frame_array.current_frame].semaphore_render_finished, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT);
	frame_array.frames[frame_array.current_frame].semaphore_render_finished.wait_counter += 1;

	const VkSubmitInfo2 submit_info = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = NULL,
		.flags = 0,
		.waitSemaphoreInfoCount = 3,
		.pWaitSemaphoreInfos = wait_semaphore_submit_infos,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &command_buffer_submit_info,
		.signalSemaphoreInfoCount = 1,
		.pSignalSemaphoreInfos = signal_semaphore_submit_infos
	};

	vkQueueSubmit2(graphics_queue, 1, &submit_info, frame_array.frames[frame_array.current_frame].fence_frame_ready);



	const VkPresentInfoKHR present_info = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &frame_array.frames[frame_array.current_frame].semaphore_present_ready.semaphore,
		.swapchainCount = 1,
		.pSwapchains = &swapchain.handle,
		.pImageIndices = &image_index,
		.pResults = NULL
	};

	timeline_to_binary_semaphore_signal(graphics_queue, frame_array.frames[frame_array.current_frame].semaphore_render_finished, frame_array.frames[frame_array.current_frame].semaphore_present_ready);
	vkQueuePresentKHR(present_queue, &present_info);

	frame_array.current_frame = (frame_array.current_frame + 1) % frame_array.num_frames;
}
