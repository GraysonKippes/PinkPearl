#include "Draw.h"

#include <stdlib.h>

#include "log/Logger.h"

#include "Descriptor.h"
#include "frame.h"
#include "texture_manager.h"
#include "TextureState.h"
#include "VulkanManager.h"

typedef struct DrawInfo {
	
	/* Indirect draw command parameters */
	
	uint32_t indexCount;
	uint32_t instanceCount;	// Unused
	uint32_t firstIndex;
	int32_t vertexOffset;
	uint32_t firstInstance;	// Unused
	
	/* Additional draw parameters */
	
	// Indexes into a model pool's array of models.
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
	
	// Some part of some buffer to which to upload draw command parameters.
	BufferSubrange drawInfoBuffer;
	
	// The graphics pipeline with which the models will be drawn.
	GraphicsPipeline graphicsPipeline;
	
	// Each pool has its own sub-array of vertices within the vertex buffer(s); this is the position of the first element in that sub-array.
	uint32_t firstVertex;
	
	// Number of vertices of each model in this pool, e.g. four for quads/rects.
	uint32_t vertexCount;
	
	// Each pool has its own sub-array of indices within the index buffer(s); this is the position of the first element in that sub-array.
	uint32_t firstIndex;
	
	// Number of indices of each model in this pool.
	uint32_t indexCount;
	
	// The descriptor index of the first model in this pool. Offsets all descriptor/model indices.
	uint32_t firstDescriptorIndex;
	
	uint32_t drawInfoBufferHandle;
	
	uint32_t matrixBufferHandle;
	
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

const uint32_t drawCountSize = sizeof(uint32_t);

const uint32_t drawCommandStride = sizeof(DrawInfo);

void createModelPool(const ModelPoolCreateInfo createInfo, ModelPool *const pOutModelPool) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating model pool...");
	
	ModelPool modelPool = calloc(1, sizeof(struct ModelPool_T));
	if (!modelPool) {
		return;
	}
	
	modelPool->graphicsPipeline = createInfo.graphicsPipeline;
	modelPool->firstVertex = createInfo.firstVertex;
	modelPool->vertexCount = createInfo.vertexCount;
	modelPool->firstIndex = createInfo.firstIndex;
	modelPool->indexCount = createInfo.indexCount;
	modelPool->firstDescriptorIndex = createInfo.firstDescriptorIndex;
	modelPool->maxModelCount = createInfo.maxModelCount;
	modelPool->drawInfoCount = 0;
	
	bufferBorrowSubrange(createInfo.buffer, createInfo.bufferSubrangeIndex, &modelPool->drawInfoBuffer);
	modelPool->drawInfoBufferHandle = uploadUniformBuffer2(device, modelPool->drawInfoBuffer);
	
	modelPool->pSlotFlags = calloc(createInfo.maxModelCount, sizeof(bool));
	if (!modelPool->pSlotFlags) {
		free(modelPool);
		return;
	}
	
	modelPool->pDrawInfoIndices = calloc(createInfo.maxModelCount, sizeof(uint32_t));
	if (!modelPool->pDrawInfoIndices) {
		free(modelPool->pSlotFlags);
		free(modelPool);
		return;
	}
	
	modelPool->pModelTransforms = calloc(createInfo.maxModelCount, sizeof(ModelTransform));
	if (!modelPool->pModelTransforms) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool);
		return;
	}
	
	modelPool->pTextureStates = calloc(createInfo.maxModelCount, sizeof(TextureState));
	if (!modelPool->pTextureStates) {
		free(modelPool->pSlotFlags);
		free(modelPool->pDrawInfoIndices);
		free(modelPool->pModelTransforms);
		free(modelPool);
		return;
	}
	
	modelPool->pDrawInfos = calloc(createInfo.maxModelCount, sizeof(DrawInfo));
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
	
	bufferReturnSubrange(&(*pModelPool)->drawInfoBuffer);
	
	free((*pModelPool)->pSlotFlags);
	free((*pModelPool)->pDrawInfoIndices);
	free((*pModelPool)->pModelTransforms);
	free((*pModelPool)->pTextureStates);
	free((*pModelPool)->pDrawInfos);
	free((*pModelPool));
	
	*pModelPool = nullptr;
}

uint32_t modelPoolGetMaxModelCount(const ModelPool modelPool) {
	return modelPool->maxModelCount;
}

VkDescriptorBufferInfo modelPoolGetBufferDescriptorInfo(const ModelPool modelPool) {
	return makeDescriptorBufferInfo2(modelPool->drawInfoBuffer);
}

void modelPoolGetDrawCommandArguments(const ModelPool modelPool, uint32_t *const pMaxDrawCount, uint32_t *const pStride) {
	*pMaxDrawCount = modelPool->maxModelCount;
	*pStride = sizeof(DrawInfo);
}

VkDeviceSize modelPoolGetDrawInfoBufferOffset(const ModelPool modelPool) {
	return modelPool->drawInfoBuffer.offset;
}

uint32_t modelPoolGetDrawInfoBufferHandle(const ModelPool modelPool) {
	return modelPool->drawInfoBufferHandle;
}

void loadModel(const ModelLoadInfo loadInfo, int *const pModelHandle) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Loading model...");
	
	// For simplicity's sake, just use a linear search to find the first available slot.
	// TODO - maybe optimize this with a data structure?
	uint32_t modelIndex = 0;
	for (; modelIndex < loadInfo.modelPool->maxModelCount; ++modelIndex) {
		if (!loadInfo.modelPool->pSlotFlags[modelIndex]) {
			break;
		}
	}
	
	if (modelIndex >= loadInfo.modelPool->maxModelCount) {
		*pModelHandle = -1;
		return;
	}
	
	loadInfo.modelPool->pModelTransforms[modelIndex] = makeModelTransform(loadInfo.position, zeroVec4F, zeroVec4F);
	
	/* Generate model's mesh and upload to vertex buffer(s) */
	
	// Compute size of mesh
	// Copy attributes into right spots
	
	static const VkDeviceSize numVerticesPerQuad = 4;
	
	float mesh[numVerticesPerQuad * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride];
	
	if (loadInfo.modelPool->graphicsPipeline.vertexAttributePositionOffset >= 0) {
		uint32_t baseOffset = 0 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributePositionOffset;
		mesh[baseOffset + 0] = loadInfo.dimensions.x1;
		mesh[baseOffset + 1] = loadInfo.dimensions.y1;
		mesh[baseOffset + 2] = 0.0F;
		baseOffset = 1 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributePositionOffset;
		mesh[baseOffset + 0] = loadInfo.dimensions.x1;
		mesh[baseOffset + 1] = loadInfo.dimensions.y2;
		mesh[baseOffset + 2] = 0.0F;
		baseOffset = 2 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributePositionOffset;
		mesh[baseOffset + 0] = loadInfo.dimensions.x2;
		mesh[baseOffset + 1] = loadInfo.dimensions.y2;
		mesh[baseOffset + 2] = 0.0F;
		baseOffset = 3 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributePositionOffset;
		mesh[baseOffset + 0] = loadInfo.dimensions.x2;
		mesh[baseOffset + 1] = loadInfo.dimensions.y1;
		mesh[baseOffset + 2] = 0.0F;
	}
	
	if (loadInfo.modelPool->graphicsPipeline.vertexAttributeTextureCoordinatesOffset >= 0) {
		uint32_t baseOffset = 0 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributeTextureCoordinatesOffset;
		mesh[baseOffset + 0] = 0.0F;
		mesh[baseOffset + 1] = 1.0F;
		baseOffset = 1 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributeTextureCoordinatesOffset;
		mesh[baseOffset + 0] = 0.0F;
		mesh[baseOffset + 1] = 0.0F;
		baseOffset = 2 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributeTextureCoordinatesOffset;
		mesh[baseOffset + 0] = 1.0F;
		mesh[baseOffset + 1] = 0.0F;
		baseOffset = 3 * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributeTextureCoordinatesOffset;
		mesh[baseOffset + 0] = 1.0F;
		mesh[baseOffset + 1] = 1.0F;
	}
	
	if (loadInfo.modelPool->graphicsPipeline.vertexAttributeColorOffset >= 0) {
		for (uint32_t i = 0; i < numVerticesPerQuad; ++i) {
			uint32_t baseOffset = i * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride + loadInfo.modelPool->graphicsPipeline.vertexAttributeColorOffset;
			mesh[baseOffset + 0] = loadInfo.color.x;
			mesh[baseOffset + 1] = loadInfo.color.y;
			mesh[baseOffset + 2] = loadInfo.color.z;
		}
	}
	
	// Upload model's mesh to staging buffer.
	// The offset of the mesh's vertices in the vertex buffer, in bytes.
	const VkDeviceSize meshOffset = sizeof(mesh) * modelIndex + loadInfo.modelPool->firstVertex * loadInfo.modelPool->graphicsPipeline.vertexInputElementStride * sizeof(float);	
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
		.indexCount = loadInfo.modelPool->indexCount,
		.instanceCount = 1,
		.firstIndex = loadInfo.modelPool->firstIndex,
		.vertexOffset = loadInfo.modelPool->firstVertex + loadInfo.modelPool->vertexCount * modelIndex,
		.firstInstance = 0,
		.modelIndex = modelIndex,
		.imageIndex = loadInfo.imageIndex
	};
	
	uint32_t insertIndex = loadInfo.modelPool->drawInfoCount;
	for (uint32_t i = 0; i < loadInfo.modelPool->drawInfoCount; ++i) {
		const float otherZ = loadInfo.modelPool->pModelTransforms[loadInfo.modelPool->pDrawInfos[i].modelIndex].translation.current.z;
		if (loadInfo.position.z > otherZ) {
			insertIndex = i;
			break;
		}
	}
	
	for (uint32_t i = loadInfo.modelPool->drawInfoCount; i > insertIndex; --i) {
		loadInfo.modelPool->pDrawInfos[i] = loadInfo.modelPool->pDrawInfos[i - 1];
		loadInfo.modelPool->pDrawInfoIndices[loadInfo.modelPool->pDrawInfos[i].modelIndex] = i;
	}
	
	loadInfo.modelPool->pDrawInfos[insertIndex] = drawInfo;
	loadInfo.modelPool->pDrawInfoIndices[modelIndex] = insertIndex;
	loadInfo.modelPool->drawInfoCount += 1;
	
	// Upload new draw info array to buffer
	bufferHostTransfer(loadInfo.modelPool->drawInfoBuffer, 0, drawCountSize, &loadInfo.modelPool->drawInfoCount);
	bufferHostTransfer(loadInfo.modelPool->drawInfoBuffer, drawCountSize, loadInfo.modelPool->drawInfoCount * sizeof(DrawInfo), loadInfo.modelPool->pDrawInfos);
	
	// Update texture descriptor
	// True if the model uses a texture, false otherwise.
	const bool textureNeeded = loadInfo.modelPool->graphicsPipeline.vertexAttributeTextureCoordinatesOffset >= 0;
	if (textureNeeded) {
		const TextureState textureState = newTextureState(loadInfo.textureID);
		loadInfo.modelPool->pTextureStates[modelIndex] = textureState;
		const Texture texture = getTexture(textureState.textureHandle);
		uploadSampledImage(device, texture.image);
	}
	
	loadInfo.modelPool->pSlotFlags[modelIndex] = true;
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
	
	bufferHostTransfer(modelPool->drawInfoBuffer, 0, drawCountSize, &modelPool->drawInfoCount);
	bufferHostTransfer(modelPool->drawInfoBuffer, drawCountSize, modelPool->drawInfoCount * sizeof(DrawInfo), modelPool->pDrawInfos);
	
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
	return &modelPool->pTextureStates[modelHandle];
}

void updateDrawInfo(ModelPool modelPool, const int modelHandle, const unsigned int imageIndex) {
	
	const uint32_t modelIndex = (uint32_t)modelHandle;
	const uint32_t drawInfoIndex = modelPool->pDrawInfoIndices[modelIndex];
	modelPool->pDrawInfos[drawInfoIndex].imageIndex = (uint32_t)imageIndex;
	
	bufferHostTransfer(modelPool->drawInfoBuffer, drawCountSize + drawInfoIndex * sizeof(DrawInfo), sizeof(DrawInfo), &modelPool->pDrawInfos[drawInfoIndex]);
}

ModelTransform *getModelTransforms(const ModelPool modelPool) {
	return modelPool->pModelTransforms;
}