#include "compute_room_texture.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/Logger.h"
#include "render/render_config.h"
#include "util/allocate.h"

#include "../CommandBuffer.h"
#include "../ComputePipeline.h"
#include "../descriptor.h"
#include "../image.h"
#include "../texture.h"
#include "../texture_manager.h"
#include "../vulkan_manager.h"



static const DescriptorBinding compute_room_texture_bindings[3] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const DescriptorSetLayout compute_room_texture_layout = {
	.num_bindings = 3,
	.bindings = (DescriptorBinding *)compute_room_texture_bindings
};

static ComputePipeline compute_room_texture_pipeline;

static Image transferImage;
static VkDeviceMemory transferImageMemory;

// Subresource range used in all image views and layout transitions.
static const ImageSubresourceRange imageSubresourceRange = {
	.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseArrayLayer = 0,
	.arrayLayerCount = VK_REMAINING_ARRAY_LAYERS
};



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
			*physical_device.queue_family_indices.transfer_family_ptr,
			*physical_device.queue_family_indices.compute_family_ptr,
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
		.arrayLayers = num_room_layers,
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
	VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
	allocCmdBufs(vkDevice, commandPoolGraphics.vkCommandPool, 1, &cmdBuf);
	cmdBufBegin(cmdBuf, true); {
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
	} vkEndCommandBuffer(cmdBuf);
	submit_command_buffers_async(queueGraphics, 1, &cmdBuf);
	transferImage.usage = imageUsageComputeWrite;
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done creating texture stitching transfer image.");
}

void init_compute_room_texture(const VkDevice vkDevice) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Initializing texture stitcher...");
	compute_room_texture_pipeline = create_compute_pipeline(vkDevice, compute_room_texture_layout, ROOM_TEXTURE_SHADER_NAME);
	createTransferImage(vkDevice);
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done initializing texture stitcher.");
}

void terminate_compute_room_texture(void) {
	destroy_compute_pipeline(&compute_room_texture_pipeline);
}

void computeStitchTexture(const int tilemapTextureHandle, const int destinationTextureHandle, const ImageSubresourceRange destinationRange, const Extent tileExtent, uint32_t **tileIndices) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Computing room texture...");
	
	const Texture tilemapTexture = getTexture(tilemapTextureHandle);
	Texture *const pRoomTexture = getTextureP(destinationTextureHandle);
	if (!pRoomTexture) {
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

	const VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = compute_room_texture_pipeline.descriptor_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &compute_room_texture_pipeline.descriptor_set_layout
	};

	VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
	const VkResult allocate_descriptor_set_result = vkAllocateDescriptorSets(device, &descriptor_set_allocate_info, &descriptor_set);
	if (allocate_descriptor_set_result != 0) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error computing room texture: descriptor set allocation failed. (Error code: %i)", allocate_descriptor_set_result);
		return;
	}

	const VkDescriptorBufferInfo uniform_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 1);
	const VkDescriptorImageInfo tilemapTexture_info = makeDescriptorImageInfo(no_sampler, tilemapTexture.image.vkImageView, tilemapTexture.image.usage.imageLayout);
	const VkDescriptorImageInfo room_texture_storage_info = makeDescriptorImageInfo(imageSamplerDefault, transferImage.vkImageView, transferImage.usage.imageLayout);
	const VkDescriptorImageInfo storage_image_infos[2] = { tilemapTexture_info, room_texture_storage_info };

	VkWriteDescriptorSet write_descriptor_sets[2] = { { 0 } };
	
	write_descriptor_sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[0].pNext = nullptr;
	write_descriptor_sets[0].dstSet = descriptor_set;
	write_descriptor_sets[0].dstBinding = 0;
	write_descriptor_sets[0].dstArrayElement = 0;
	write_descriptor_sets[0].descriptorCount = 1;
	write_descriptor_sets[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write_descriptor_sets[0].pImageInfo = nullptr;
	write_descriptor_sets[0].pBufferInfo = &uniform_buffer_info;
	write_descriptor_sets[0].pTexelBufferView = nullptr;

	write_descriptor_sets[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write_descriptor_sets[1].pNext = nullptr;
	write_descriptor_sets[1].dstSet = descriptor_set;
	write_descriptor_sets[1].dstBinding = 1;
	write_descriptor_sets[1].dstArrayElement = 0;
	write_descriptor_sets[1].descriptorCount = 2;
	write_descriptor_sets[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	write_descriptor_sets[1].pImageInfo = storage_image_infos;
	write_descriptor_sets[1].pBufferInfo = nullptr;
	write_descriptor_sets[1].pTexelBufferView = nullptr;

	vkUpdateDescriptorSets(device, 2, write_descriptor_sets, 0, nullptr);

	// Run compute shader to stitch texture.
	VkCommandBuffer cmdBufCompute = VK_NULL_HANDLE;
	allocCmdBufs(device, commandPoolCompute.vkCommandPool, 1, &cmdBufCompute);
	cmdBufBegin(cmdBufCompute, true); {
		vkCmdBindPipeline(cmdBufCompute, VK_PIPELINE_BIND_POINT_COMPUTE, compute_room_texture_pipeline.handle);
		vkCmdBindDescriptorSets(cmdBufCompute, VK_PIPELINE_BIND_POINT_COMPUTE, compute_room_texture_pipeline.layout, 0, 1, &descriptor_set, 0, nullptr);
		vkCmdDispatch(cmdBufCompute, tileExtent.width, tileExtent.length, num_room_layers);
	} vkEndCommandBuffer(cmdBufCompute);
	submit_command_buffers_async(queueCompute, 1, &cmdBufCompute);
	vkFreeCommandBuffers(device, commandPoolCompute.vkCommandPool, 1, &cmdBufCompute);
	
	// Perform the transfer to the target texture image.
	VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
	allocCmdBufs(device, commandPoolGraphics.vkCommandPool, 1, &cmdBuf);
	cmdBufBegin(cmdBuf, true); {
		
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
		
	} vkEndCommandBuffer(cmdBuf);
	submit_command_buffers_async(queueGraphics, 1, &cmdBuf);
	vkFreeCommandBuffers(device, commandPoolGraphics.vkCommandPool, 1, &cmdBuf);

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Done computing room texture.");
}
