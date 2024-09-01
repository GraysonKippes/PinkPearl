#include "Draw.h"

#include <stdlib.h>

#include "log/Logger.h"

#include "frame.h"
#include "texture_manager.h"
#include "TextureState.h"
#include "vertex_input.h"
#include "vulkan_manager.h"

typedef struct DrawInfo {
	
	/* Indirect draw command parameters */
	
	uint32_t indexCount;
	uint32_t instanceCount;	// IGNORED
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;	// IGNORED
	
	/* Additional draw parameters */
	
	// Indexes into global descriptor arrays.
	int32_t modelIndex;
	
	// Indexes into a texture array.
	uint32_t imageIndex;
	
} DrawInfo;

// Holds data for a batch of models to be drawn with a single indirect draw call.
// Controls how the parameters for the indirect draw call are generated.
struct ModelPool_T {
	
	// In the current system, all models in the same pool use the same structure, aka, the same indices with the same number of vertices.
	// As a result, the positions of the first vertex and first index for a particular model pool in the vertex/index buffer(s) is constant
	// 	and the position of a particular mesh can be calculated by multiplying the mesh size by its relative position in the mesh array.
	
	// Each pool has its own sub-array of vertices within the vertex buffer(s); this is the position of the first element in that sub-array.
	uint32_t firstVertex;
	
	// Number of vertices of each model in this pool, e.g. four for quads/rects.
	uint32_t vertexCount;
	
	// Each pool has its own sub-array of indices within the index buffer(s); this is the position of the first element in that sub-array.
	uint32_t firstIndex;
	
	// Number of indices of each model in this pool.
	uint32_t indexCount;
	
	// The maximum number of models.
	uint32_t maxModelCount;
	
	/* MODEL OBJECT FIELDS */
	
	// Indicates whether a model is loaded into a particular model array slot.
	bool *pSlotFlags;
	
	// The index into the array of draw info structures uploaded to the GPU.
	// Used to update a particular model's draw parameters without rebuilding the whole array of draw info structures.
	uint32_t *pDrawInfoIndices;
	
	ModelTransform *pModelTransforms;
	
	TextureState *pTextureStates;
	
	/* DRAW INFO */
	
	// The number of draw infos, but not the space allocated.
	// Equal to the number of *active* models.
	uint32_t drawInfoCount;
	
	// All of the draw parameters for each of the models.
	// DrawInfoIndex indexes into this array.
	DrawInfo *pDrawInfos;
};

void createModelPool(const uint32_t firstVertex, const uint32_t vertexCount, const uint32_t firstIndex, const uint32_t indexCount, const uint32_t maxModelCount, ModelPool *pOutModelPool) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating model pool...");
	
	ModelPool modelPool = calloc(1, sizeof(struct ModelPool_T));
	if (!modelPool) {
		return;
	}
	
	modelPool->firstVertex = firstVertex;
	modelPool->vertexCount = vertexCount;
	modelPool->firstIndex = firstIndex;
	modelPool->indexCount = indexCount;
	modelPool->maxModelCount = maxModelCount;
	modelPool->drawInfoCount = 0;
	
	modelPool->pSlotFlags = calloc(maxModelCount, sizeof(bool));
	if (!modelPool->pSlotFlags) {
		free(modelPool);
		return;
	}
	
	modelPool->pDrawInfoIndices = calloc(maxModelCount, sizeof(uint32_t));
	if (!modelPool->pDrawInfoIndices) {
		free(modelPool->pSlotFlags);
		free(modelPool);
		return;
	}
	
	modelPool->pModelTransforms = calloc(maxModelCount, sizeof(ModelTransform));
	if (!modelPool->pModelTransforms) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool);
		return;
	}
	
	modelPool->pTextureStates = calloc(maxModelCount, sizeof(TextureState));
	if (!modelPool->pTextureStates) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool->pModelTransforms);
		free(modelPool);
		return;
	}
	
	modelPool->pDrawInfos = calloc(maxModelCount, sizeof(DrawInfo));
	if (!modelPool->pTextureStates) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool->pModelTransforms);
		free(modelPool->pTextureStates);
		free(modelPool);
		return;
	}
	
	*pOutModelPool = modelPool;
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Created model pool.");
}

void deleteModelPool(ModelPool *const pModelPool) {
	free((*pModelPool)->pSlotFlags);
	free((*pModelPool)->pDrawInfoIndices);
	free((*pModelPool)->pModelTransforms);
	free((*pModelPool)->pTextureStates);
	free((*pModelPool)->pDrawInfos);
	free((*pModelPool));
	*pModelPool = nullptr;
}

void modelPoolGetDrawCommandArguments(const ModelPool modelPool, uint32_t *const pMaxDrawCount, uint32_t *const pStride) {
	*pMaxDrawCount = modelPool->maxModelCount;
	*pStride = sizeof(DrawInfo);
}

void loadModel(ModelPool modelPool, const Vector4F position, const BoxF dimensions, const String textureID, int *const pModelHandle) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Loading model...");
	
	// For simplicity's sake, just use a linear search to find the first available slot.
	// TODO - maybe optimize this with a data structure?
	uint32_t modelIndex = 0;
	for (; modelIndex < modelPool->maxModelCount; ++modelIndex) {
		if (!modelPool->pSlotFlags[modelIndex]) {
			break;
		}
	}
	
	if (modelIndex >= modelPool->maxModelCount) {
		*pModelHandle = -1;
		return;
	}
	
	// Reset model object fields.
	modelPool->pModelTransforms[modelIndex] = makeModelTransform(position, zeroVector4F, zeroVector4F);
	TextureState textureState = newTextureState(textureID);
	modelPool->pTextureStates[modelIndex] = textureState;	
	
	/* Generate model's mesh and upload to vertex buffer(s) */
	
	// Generate model's mesh.
	const float mesh[NUM_VERTICES_PER_QUAD * VERTEX_INPUT_ELEMENT_STRIDE] = {
		// Positions							Texture			Color
		dimensions.x1, dimensions.y1, 0.0F,		0.0F, 1.0F,		1.0F, 1.0F, 1.0F,	// 0: Bottom-left
		dimensions.x1, dimensions.y2, 0.0F,		0.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// 1: Top-left
		dimensions.x2, dimensions.y2, 0.0F,		1.0F, 0.0F,		1.0F, 1.0F, 1.0F,	// 2: Top-right
		dimensions.x2, dimensions.y1, 0.0F,		1.0F, 1.0F,		1.0F, 1.0F, 1.0F	// 3: Bottom-right
	};
	
	// Upload model's mesh to staging buffer.
	// The offset of the mesh's vertices in the vertex buffer, in bytes.
	const VkDeviceSize meshOffset = sizeof(mesh) * modelIndex + modelPool->firstVertex * vertex_input_element_stride * sizeof(float);	
	unsigned char *const pMappedMemoryVertices = buffer_partition_map_memory(global_staging_buffer_partition, 0);
	memcpy(&pMappedMemoryVertices[meshOffset], mesh, sizeof(mesh));
	buffer_partition_unmap_memory(global_staging_buffer_partition);
	
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
	
	static VkCommandBuffer cmdBufs[NUM_FRAMES_IN_FLIGHT] = { VK_NULL_HANDLE };
	vkFreeCommandBuffers(device, commandPoolTransfer.vkCommandPool, frame_array.num_frames, cmdBufs);
	allocCmdBufs(device, commandPoolTransfer.vkCommandPool, frame_array.num_frames, cmdBufs);
	
	VkCommandBufferSubmitInfo cmdBufSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { } };
	VkSemaphoreSubmitInfo semaphoreWaitSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { } };
	VkSemaphoreSubmitInfo semaphoreSignalSubmitInfos[NUM_FRAMES_IN_FLIGHT] = { { } };
	VkSubmitInfo2 submitInfos[NUM_FRAMES_IN_FLIGHT] = { { } };
	
	const VkBufferCopy bufferCopy = {
		.srcOffset = meshOffset,
		.dstOffset = meshOffset,
		.size = sizeof(mesh)
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
	
	/* Create and insert new model's draw info struct */
	
	const DrawInfo drawInfo = {
		.indexCount = modelPool->indexCount,
		.instanceCount = 1,
		.firstIndex = modelPool->firstIndex,
		.vertexOffset = modelPool->firstVertex + modelPool->vertexCount * modelIndex,
		.firstInstance = 0,
		.modelIndex = modelIndex,
		.imageIndex = 0
	};
	
	uint32_t insertIndex = modelPool->drawInfoCount;
	for (uint32_t i = 0; i < modelPool->drawInfoCount; ++i) {
		const float otherZ = modelPool->pModelTransforms[modelPool->pDrawInfos[i].modelIndex].translation.current.z;
		if (position.z > otherZ) {
			insertIndex = i;
			break;
		}
	}
	
	for (uint32_t i = modelPool->drawInfoCount; i > insertIndex; --i) {
		modelPool->pDrawInfos[i] = modelPool->pDrawInfos[i - 1];
		modelPool->pDrawInfoIndices[modelPool->pDrawInfos[i].modelIndex] = i;
	}
	
	modelPool->pDrawInfos[insertIndex] = drawInfo;
	modelPool->pDrawInfoIndices[modelIndex] = insertIndex;
	modelPool->drawInfoCount += 1;
	
	// Upload new draw info array to buffer
	
	// TODO - cut the partition crap and just do one mapped memory.
	unsigned char *const pDrawCountMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 0);
	memcpy(pDrawCountMappedMemory, &modelPool->drawInfoCount, sizeof(modelPool->drawInfoCount));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	unsigned char *const pDrawInfosMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(pDrawInfosMappedMemory, modelPool->pDrawInfos, modelPool->drawInfoCount * sizeof(*modelPool->pDrawInfos));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	// Update texture descriptor
	
	const Texture texture = getTexture(textureState.textureHandle);
	VkWriteDescriptorSet writeDescriptorSets[NUM_FRAMES_IN_FLIGHT] = { { } };
	
	// Replace malloc with a better suited allocation, perhaps an arena?
	VkDescriptorImageInfo *pDescriptorImageInfo = malloc(sizeof(VkDescriptorImageInfo));
	*pDescriptorImageInfo = makeDescriptorImageInfo(texture.image);
	
	for (uint32_t i = 0; i < frame_array.num_frames; ++i) {
		writeDescriptorSets[i] = (VkWriteDescriptorSet){
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet = frame_array.frames[i].descriptorSet.vkDescriptorSet,
			.dstBinding = 2,
			.dstArrayElement = modelIndex,
			.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
			.descriptorCount = 1,
			.pBufferInfo = nullptr,
			.pImageInfo = pDescriptorImageInfo,
			.pTexelBufferView = nullptr
		};
		descriptorSetPushWrite(&frame_array.frames[i].descriptorSet, writeDescriptorSets[i]);
	}
	
	modelPool->pSlotFlags[modelIndex] = true;
	*pModelHandle = (int)modelIndex;
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Loaded model %i.", *pModelHandle);
}

void unloadModel(ModelPool modelPool, int *const pModelHandle) {
	
	// Remove model's draw info from model pool's array of draw infos
	// If any other draw infos are moved around, adjust their models' draw info indices
	
	// Get the index of the draw data associated with the quad.
	const uint32_t modelIndex = (uint32_t)*pModelHandle;
	const uint32_t drawInfoIndex = modelPool->pDrawInfoIndices[modelIndex];
	if (drawInfoIndex >= modelPool->drawInfoCount) {
		return;
	}
	
	// Remove the draw data associated with the quad.
	modelPool->drawInfoCount -= 1;
	// Move each draw data ahead of the removed draw data forward one place.
	for (uint32_t i = drawInfoIndex; i < modelPool->drawInfoCount; ++i) {
		// i = index of the draw data after being moved forward one place.
		// i + 1 = index of the draw data before being moved forward one place.
		modelPool->pDrawInfos[i] = modelPool->pDrawInfos[i + 1];	// Copy the next draw data into this place.
		modelPool->pDrawInfoIndices[modelPool->pDrawInfos[i].modelIndex] = i;
	}
	
	// TODO - cut the partition crap and just do one mapped memory.
	unsigned char *const pDrawCountMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 0);
	memcpy(pDrawCountMappedMemory, &modelPool->drawInfoCount, sizeof(modelPool->drawInfoCount));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	unsigned char *const pDrawInfosMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(pDrawInfosMappedMemory, modelPool->pDrawInfos, modelPool->drawInfoCount * sizeof(*modelPool->pDrawInfos));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
	
	modelPool->pSlotFlags[modelIndex] = false;
	
	*pModelHandle = -1;
}

void modelSetTranslation(ModelPool modelPool, const int modelHandle, const Vector4F translation) {
	renderVectorSet(&modelPool->pModelTransforms[modelHandle].translation, translation);
}

void modelSetScaling(ModelPool modelPool, const int modelHandle, const Vector4F scaling) {
	renderVectorSet(&modelPool->pModelTransforms[modelHandle].scaling, scaling);
}

void modelSetRotation(ModelPool modelPool, const int modelHandle, const Vector4F rotation) {
	renderVectorSet(&modelPool->pModelTransforms[modelHandle].rotation, rotation);
}

TextureState *modelGetTextureState(ModelPool modelPool, const int modelHandle) {
	//logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Getting texture state from model %i...", modelHandle);
	return &modelPool->pTextureStates[modelHandle];
}

void updateDrawInfo(ModelPool modelPool, const int modelHandle, const unsigned int imageIndex) {
	//logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Updating draw info for model %i...", modelHandle);
	
	const uint32_t modelIndex = (uint32_t)modelHandle;
	const uint32_t drawInfoIndex = modelPool->pDrawInfoIndices[modelIndex];
	modelPool->pDrawInfos[drawInfoIndex].imageIndex = (uint32_t)imageIndex;
	
	uint8_t *const drawDataMappedMemory = buffer_partition_map_memory(global_draw_data_buffer_partition, 1);
	memcpy(&drawDataMappedMemory[drawInfoIndex * sizeof(DrawInfo)], &modelPool->pDrawInfos[drawInfoIndex], sizeof(DrawInfo));
	buffer_partition_unmap_memory(global_draw_data_buffer_partition);
}

ModelTransform *getModelTransforms(const ModelPool modelPool) {
	return modelPool->pModelTransforms;
}