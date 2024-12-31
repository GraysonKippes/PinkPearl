#include "ComputeStitchTexture.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "log/Logger.h"
#include "render/render_config.h"
#include "util/Allocation.h"
#include "../CommandBuffer.h"
#include "../ComputePipeline.h"
#include "../Descriptor.h"
#include "../texture.h"
#include "../texture_manager.h"
#include "../VulkanManager.h"



// Subresource range used in all image views and layout transitions.
static const ImageSubresourceRange imageSubresourceRange = {
	.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseArrayLayer = 0,
	.arrayLayerCount = VK_REMAINING_ARRAY_LAYERS
};

static Pipeline computeRoomTexturePipeline;

static CmdBufArray stitchTextureCmdBufArray = { };
static CmdBufArray transferImageCmdBufArray = { };

static VkFence stitchTextureFence = VK_NULL_HANDLE;
static VkFence transferImageFence = VK_NULL_HANDLE;

static Image transferImage;
static VkDeviceMemory transferImageMemory;

static uint32_t uniformBufferDescriptorHandle = DESCRIPTOR_HANDLE_INVALID;
static uint32_t transferImageDescriptorHandle = DESCRIPTOR_HANDLE_INVALID;



static void createTransferImage(const VkDevice vkDevice) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating texture stitching transfer image...");
	
	transferImage = (Image){
		.vkImage = VK_NULL_HANDLE,
		.vkImageView = VK_NULL_HANDLE,
		.vkFormat = VK_FORMAT_UNDEFINED,
		.usage = imageUsageUndefined
	};
	
	// Dimensions for the largest possible room texture, smaller textures use a subsection of this image.
	const Extent largestRoomExtent = room_size_to_extent((RoomSize)(num_room_sizes - 1));
	const Extent imageDimensions = extent_scale(largestRoomExtent, tile_texel_length);
	
	const queue_family_set_t queueFamilyIndexSet = {
		.num_queue_families = 2,
		.queue_families = (uint32_t[2]){
			*physical_device.queueFamilyIndices.transfer_family_ptr,
			*physical_device.queueFamilyIndices.compute_family_ptr,
		}
	};
	
	// Create the image.
	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = VK_FORMAT_R8G8B8A8_UINT,
		.extent.width = imageDimensions.width,
		.extent.height = imageDimensions.length,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = numRoomLayers,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
		.sharingMode = VK_SHARING_MODE_CONCURRENT,
		.queueFamilyIndexCount = queueFamilyIndexSet.num_queue_families,
		.pQueueFamilyIndices = queueFamilyIndexSet.queue_families,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};
	vkCreateImage(vkDevice, &imageCreateInfo, nullptr, &transferImage.vkImage);
	transferImage.vkFormat = VK_FORMAT_R8G8B8A8_UINT;
	transferImage.extent = imageDimensions;
	
	// Allocate memory for the image and bind the image to it.
	const VkMemoryRequirements2 imageMemoryRequirements = getImageMemoryRequirements(vkDevice, transferImage.vkImage);
	const VkMemoryAllocateInfo allocateInfo = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = nullptr,
		.allocationSize = imageMemoryRequirements.memoryRequirements.size,
		.memoryTypeIndex = memory_type_set.graphics_resources
	};
	vkAllocateMemory(vkDevice, &allocateInfo, nullptr, &transferImageMemory);
	bindImageMemory(vkDevice, transferImage.vkImage, transferImageMemory, 0);
	
	// Create the image view.
	const VkImageViewCreateInfo imageViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = transferImage.vkImage,
		.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
		.format = transferImage.vkFormat,
		.components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
		.components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		.subresourceRange = makeImageSubresourceRange(imageSubresourceRange)
	};
	vkCreateImageView(vkDevice, &imageViewCreateInfo, nullptr, &transferImage.vkImageView);
	
	// Transition the image layout.
	CmdBufArray cmdBufArray = cmdBufAlloc(commandPoolGraphics, 1);
	recordCommands(cmdBufArray, 0, true,
		const VkImageMemoryBarrier2 imageMemoryBarrier = makeImageTransitionBarrier(transferImage, imageSubresourceRange, imageUsageComputeWrite);
		const VkDependencyInfo dependencyInfo = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &imageMemoryBarrier
		};
		vkCmdPipelineBarrier2(cmdBuf, &dependencyInfo);
	);
	submit_command_buffers_async(queueGraphics, 1, &cmdBufArray.pCmdBufs[0]);
	cmdBufFree(&cmdBufArray);
	transferImage.usage = imageUsageComputeWrite;
	
	transferImage.vkDevice = vkDevice;
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Created texture stitching transfer image.");
}

void initComputeStitchTexture(const VkDevice vkDevice) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Initializing texture stitcher...");
	
	const ComputePipelineCreateInfo pipelineCreateInfo = {
		.vkDevice = vkDevice,
		.shaderFilename = makeStaticString("RoomTexture.spv"),
		.pushConstantRangeCount = 1,
		.pPushConstantRanges = (PushConstantRange[1]){
			{
				.shaderStageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
				.size = 3 * sizeof(uint32_t)
			}
		}
	};
	computeRoomTexturePipeline = createComputePipeline(pipelineCreateInfo);
	
	createTransferImage(vkDevice);
	
	stitchTextureCmdBufArray = cmdBufAlloc(commandPoolCompute, 1);
	transferImageCmdBufArray = cmdBufAlloc(commandPoolGraphics, 1);
	
	const VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = VK_FENCE_CREATE_SIGNALED_BIT
	};
	vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &stitchTextureFence);
	vkCreateFence(vkDevice, &fenceCreateInfo, nullptr, &transferImageFence);
	
	uniformBufferDescriptorHandle = uploadUniformBuffer(vkDevice, global_uniform_buffer_partition, 1);
	transferImageDescriptorHandle = uploadStorageImage(vkDevice, transferImage);
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done initializing texture stitcher.");
}

void terminateComputeStitchTexture(void) {
	vkFreeMemory(transferImage.vkDevice, transferImageMemory, nullptr);
	vkDestroyFence(transferImage.vkDevice, stitchTextureFence, nullptr);
	vkDestroyFence(transferImage.vkDevice, transferImageFence, nullptr);
	cmdBufFree(&stitchTextureCmdBufArray);
	cmdBufFree(&transferImageCmdBufArray);
	deleteImage(&transferImage);
	deletePipeline(&computeRoomTexturePipeline);
}

void computeStitchTexture(const int tilemapTextureHandle, const int destinationTextureHandle, const ImageSubresourceRange destinationRange, const Extent tileExtent, uint32_t **tileIndices) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Stitching texture...");
	
	const Texture tilemapTexture = getTexture(tilemapTextureHandle);
	Texture *const pRoomTexture = getTextureP(destinationTextureHandle);
	if (!pRoomTexture) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Stitching texture: could not get room texture.");
		return;
	}
	
	byte_t *mappedMemory = buffer_partition_map_memory(global_uniform_buffer_partition, 1); {
		const uint32_t numTileIndices = extentArea(tileExtent);
		const uint32_t layerSize = numTileIndices * sizeof(**tileIndices);
		const uint32_t fullLayerSize = 640 * sizeof(**tileIndices);
		for (uint32_t i = 0; i < destinationRange.arrayLayerCount; ++i) {
			memcpy(&mappedMemory[fullLayerSize * i], tileIndices[i], layerSize);
		}
	} buffer_partition_unmap_memory(global_uniform_buffer_partition);
	
	const uint32_t tilemapImageDescriptorHandle = uploadStorageImage(computeRoomTexturePipeline.vkDevice, tilemapTexture.image);

	// Run compute shader to stitch texture.
	vkWaitForFences(computeRoomTexturePipeline.vkDevice, 1, &stitchTextureFence, VK_TRUE, UINT64_MAX);
	vkResetFences(computeRoomTexturePipeline.vkDevice, 1, &stitchTextureFence);
	recordCommands(stitchTextureCmdBufArray, 0, false,
		vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, computeRoomTexturePipeline.vkPipeline);
		vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, computeRoomTexturePipeline.vkPipelineLayout, 0, 1, &globalDescriptorSet, 0, nullptr);
		const uint32_t pushConstants[3] = { 
			tilemapImageDescriptorHandle,
			transferImageDescriptorHandle,
			uniformBufferDescriptorHandle
		};
		vkCmdPushConstants(cmdBuf, computeRoomTexturePipeline.vkPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), pushConstants);
		vkCmdDispatch(cmdBuf, tileExtent.width, tileExtent.length, numRoomLayers);
	);
	
	const VkCommandBufferSubmitInfo stitchTextureCmdBufSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = stitchTextureCmdBufArray.pCmdBufs[0],
		.deviceMask = 0
	};
	const VkSubmitInfo2 stitchTextureSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 0,
		.pWaitSemaphoreInfos = nullptr,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &stitchTextureCmdBufSubmitInfo,
		.signalSemaphoreInfoCount = 0,
		.pSignalSemaphoreInfos = nullptr
	};
	vkQueueSubmit2(queueCompute, 1, &stitchTextureSubmitInfo, stitchTextureFence);
	vkQueueWaitIdle(queueCompute);
	
	// Perform the transfer to the target texture image.
	vkWaitForFences(computeRoomTexturePipeline.vkDevice, 1, &transferImageFence, VK_TRUE, UINT64_MAX);
	vkResetFences(computeRoomTexturePipeline.vkDevice, 1, &transferImageFence);
	recordCommands(transferImageCmdBufArray, 0, false,
		
		// Transition the compute texture to transfer source and the destination texture to transfer destination.
		const VkImageMemoryBarrier2 imageMemoryBarriers1[2] = {
			[0] = makeImageTransitionBarrier(transferImage, imageSubresourceRange, imageUsageTransferSource),
			[1] = makeImageTransitionBarrier(pRoomTexture->image, imageSubresourceRange, imageUsageTransferDestination)
		};
		const VkDependencyInfo dependencyInfo1 = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 2,
			.pImageMemoryBarriers = imageMemoryBarriers1
		};
		vkCmdPipelineBarrier2(cmdBuf, &dependencyInfo1);
		transferImage.usage = imageUsageTransferSource;
		pRoomTexture->image.usage = imageUsageTransferDestination;
	
		const ImageSubresourceRange sourceRange = {
			.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseArrayLayer = 0,
			.arrayLayerCount = destinationRange.arrayLayerCount
		};
		const VkImageCopy2 imageCopyRegion = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
			.pNext = nullptr,
			.srcSubresource = makeImageSubresourceLayers(sourceRange),
			.srcOffset = (VkOffset3D){ 0, 0, 0 },
			.dstSubresource = makeImageSubresourceLayers(destinationRange),
			.dstOffset = (VkOffset3D){ 0, 0, 0 },
			.extent = (VkExtent3D){
				.width = pRoomTexture->image.extent.width,
				.height = pRoomTexture->image.extent.length,
				.depth = 1
			}
		};
		const VkCopyImageInfo2 copyImageInfo = {
			.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
			.pNext = nullptr,
			.srcImage = transferImage.vkImage,
			.srcImageLayout = transferImage.usage.imageLayout,
			.dstImage = pRoomTexture->image.vkImage,
			.dstImageLayout = pRoomTexture->image.usage.imageLayout,
			.regionCount = 1,
			.pRegions = &imageCopyRegion
		};
		vkCmdCopyImage2(cmdBuf, &copyImageInfo);
		
		// Transition the compute texture to general and the destination texture to sampled.
		const VkImageMemoryBarrier2 imageMemoryBarriers2[2] = {
			[0] = makeImageTransitionBarrier(transferImage, imageSubresourceRange, imageUsageComputeWrite),
			[1] = makeImageTransitionBarrier(pRoomTexture->image, imageSubresourceRange, imageUsageSampled)
		};
		const VkDependencyInfo dependencyInfo2 = {
			.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
			.pNext = nullptr,
			.dependencyFlags = 0,
			.memoryBarrierCount = 0,
			.pMemoryBarriers = nullptr,
			.bufferMemoryBarrierCount = 0,
			.pBufferMemoryBarriers = nullptr,
			.imageMemoryBarrierCount = 2,
			.pImageMemoryBarriers = imageMemoryBarriers2
		};
		vkCmdPipelineBarrier2(cmdBuf, &dependencyInfo2);
		transferImage.usage = imageUsageComputeWrite;
		pRoomTexture->image.usage = imageUsageSampled;
	);
	
	const VkCommandBufferSubmitInfo transferImageCmdBufSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
		.pNext = nullptr,
		.commandBuffer = transferImageCmdBufArray.pCmdBufs[0],
		.deviceMask = 0
	};
	const VkSubmitInfo2 transferImageSubmitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
		.pNext = nullptr,
		.waitSemaphoreInfoCount = 0,
		.pWaitSemaphoreInfos = nullptr,
		.commandBufferInfoCount = 1,
		.pCommandBufferInfos = &transferImageCmdBufSubmitInfo,
		.signalSemaphoreInfoCount = 0,
		.pSignalSemaphoreInfos = nullptr
	};
	vkQueueSubmit2(queueGraphics, 1, &transferImageSubmitInfo, transferImageFence);
	vkQueueWaitIdle(queueGraphics);

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done stitching texture.");
}
