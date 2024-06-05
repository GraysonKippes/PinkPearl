#include "compute_room_texture.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "log/logging.h"
#include "render/render_config.h"
#include "util/allocate.h"

#include "../command_buffer.h"
#include "../compute_pipeline.h"
#include "../descriptor.h"
#include "../image.h"
#include "../texture.h"
#include "../texture_manager.h"
#include "../vulkan_manager.h"



static const descriptor_binding_t compute_room_texture_bindings[3] = {
	{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT },
	{ .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .count = 1, .stages = VK_SHADER_STAGE_COMPUTE_BIT }
};

static const descriptor_layout_t compute_room_texture_layout = {
	.num_bindings = 3,
	.bindings = (descriptor_binding_t *)compute_room_texture_bindings
};

static compute_pipeline_t compute_room_texture_pipeline;
static image_t computeRoomTextureTransferImage;

static Image transferImage;
static VkDeviceMemory transferImageMemory;

// Subresource range used in all image views and layout transitions.
[[deprecated]]
static const VkImageSubresourceRange image_subresource_range = {
	.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseMipLevel = 0,
	.levelCount = 1,
	.baseArrayLayer = 0,
	.layerCount = VK_REMAINING_ARRAY_LAYERS
};

static const ImageSubresourceRange imageSubresourceRange = {
	.imageAspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	.baseArrayLayer = 0,
	.arrayLayerCount = VK_REMAINING_ARRAY_LAYERS
};



static void createTransferImage(const VkDevice vkDevice) {
	log_message(VERBOSE, "Creating texture stitching transfer image...");
	
	transferImage = (Image){
		.vkImage = VK_NULL_HANDLE,
		.vkImageView = VK_NULL_HANDLE,
		.vkFormat = VK_FORMAT_UNDEFINED,
		.usage = imageUsageUndefined
	};
	
	// Dimensions for the largest possible room texture, smaller textures use a subsection of this image.
	const extent_t largestRoomExtent = room_size_to_extent((RoomSize)(num_room_sizes - 1));
	const extent_t imageDimensions = extent_scale(largestRoomExtent, tile_texel_length);
	
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
	allocate_command_buffers(vkDevice, cmdPoolGraphics, 1, &cmdBuf);
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
	
	log_message(VERBOSE, "Done creating texture stitching transfer image.");
}

void init_compute_room_texture(const VkDevice vkDevice) {
	log_message(VERBOSE, "Initializing texture stitcher...");
	compute_room_texture_pipeline = create_compute_pipeline(vkDevice, compute_room_texture_layout, ROOM_TEXTURE_SHADER_NAME);
	createTransferImage(vkDevice);
	log_message(VERBOSE, "Done initializing texture stitcher.");
}

void terminate_compute_room_texture(void) {
	destroy_image(&computeRoomTextureTransferImage);
	destroy_compute_pipeline(&compute_room_texture_pipeline);
}

void computeRoomTexture(const room_t room, const uint32_t cacheSlot, const int tilemapTextureHandle, const int roomTextureHandle) {
	log_message(VERBOSE, "Computing room texture...");
	
	const Texture tilemapTexture = getTexture(tilemapTextureHandle);
	Texture *const pRoomTexture = getTextureP(roomTextureHandle);
	if (!pRoomTexture) {
		return;
	}

	byte_t *mapped_memory = buffer_partition_map_memory(global_uniform_buffer_partition, 1);
	const uint64_t num_tiles = extent_area(room.extent);
	memcpy(mapped_memory, room.tiles, num_tiles * sizeof(tile_t));
	buffer_partition_unmap_memory(global_uniform_buffer_partition);

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
		logf_message(ERROR, "Error computing room texture: descriptor set allocation failed. (Error code: %i)", allocate_descriptor_set_result);
		return;
	}

	const VkDescriptorBufferInfo uniform_buffer_info = buffer_partition_descriptor_info(global_uniform_buffer_partition, 1);
	const VkDescriptorImageInfo tilemapTexture_info = make_descriptor_image_info(no_sampler, tilemapTexture.image.vkImageView, tilemapTexture.image.usage.imageLayout);
	const VkDescriptorImageInfo room_texture_storage_info = make_descriptor_image_info(imageSamplerDefault, transferImage.vkImageView, transferImage.usage.imageLayout);
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
	allocate_command_buffers(device, compute_command_pool, 1, &cmdBufCompute);
	cmdBufBegin(cmdBufCompute, true); {
		vkCmdBindPipeline(cmdBufCompute, VK_PIPELINE_BIND_POINT_COMPUTE, compute_room_texture_pipeline.handle);
		vkCmdBindDescriptorSets(cmdBufCompute, VK_PIPELINE_BIND_POINT_COMPUTE, compute_room_texture_pipeline.layout, 0, 1, &descriptor_set, 0, nullptr);
		vkCmdDispatch(cmdBufCompute, room.extent.width, room.extent.length, 1);
	} vkEndCommandBuffer(cmdBufCompute);
	submit_command_buffers_async(compute_queue, 1, &cmdBufCompute);
	vkFreeCommandBuffers(device, compute_command_pool, 1, &cmdBufCompute);

	[[deprecated]]
	const VkImageSubresourceRange room_texture_image_subresource_range = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = cacheSlot * num_room_layers,
		.layerCount = num_room_layers
	};

	// TODO - make the transitions and transfer all go into one command buffer submitted to the graphics queue.

	/* Transition the room texture image layout from shader read-only to transfer destination. */
	
	VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
	allocate_command_buffers(device, cmdPoolGraphics, 1, &cmdBuf);
	cmdBufBegin(cmdBuf, true); {
		const VkImageMemoryBarrier2 imageMemoryBarrier = makeImageTransitionBarrier(pRoomTexture->image, imageSubresourceRange, imageUsageTransferDestination);
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
	vkResetCommandBuffer(cmdBuf, 0);
	pRoomTexture->image.usage = imageUsageTransferDestination;

	/* Transfer compute result image to room texture image. */
	
	log_message(VERBOSE, "Transfering computed room image to room texture...");
	VkCommandBuffer transfer_command_buffer = VK_NULL_HANDLE;
	allocate_command_buffers(device, cmdPoolGraphics, 1, &transfer_command_buffer);
	cmdBufBegin(transfer_command_buffer, true); {

		const VkImageSubresourceLayers source_image_subresource_layers = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = num_room_layers
		};

		const VkImageSubresourceLayers destination_image_subresource_layers = {
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = cacheSlot * num_room_layers,
			.layerCount = num_room_layers
		};

		const VkImageCopy2 image_copy_region = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_COPY_2,
			.pNext = nullptr,
			.srcSubresource = source_image_subresource_layers,
			.srcOffset = (VkOffset3D){ 0, 0, 0 },
			.dstSubresource = destination_image_subresource_layers,
			.dstOffset = (VkOffset3D){ 0, 0, 0 },
			.extent = (VkExtent3D){
				.width = room.extent.width * tile_texel_length,
				.height = room.extent.length * tile_texel_length,
				.depth = 1
			}
		};

		const VkCopyImageInfo2 copy_image_info = {
			.sType = VK_STRUCTURE_TYPE_COPY_IMAGE_INFO_2,
			.pNext = nullptr,
			.srcImage = transferImage.vkImage,
			.srcImageLayout = transferImage.usage.imageLayout,
			.dstImage = pRoomTexture->image.vkImage,
			.dstImageLayout = pRoomTexture->image.usage.imageLayout,
			.regionCount = 1,
			.pRegions = &image_copy_region
		};

		vkCmdCopyImage2(transfer_command_buffer, &copy_image_info);

	} vkEndCommandBuffer(transfer_command_buffer);
	submit_command_buffers_async(queueGraphics, 1, &transfer_command_buffer);
	vkFreeCommandBuffers(device, cmdPoolGraphics, 1, &transfer_command_buffer);

	/* Transition the room texture image layout back from transfer destination to shader read-only. */
	
	cmdBufBegin(cmdBuf, true); {
		const VkImageMemoryBarrier2 imageMemoryBarrier = makeImageTransitionBarrier(pRoomTexture->image, imageSubresourceRange, imageUsageSampled);
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
	vkFreeCommandBuffers(device, cmdPoolGraphics, 1, &cmdBuf);
	pRoomTexture->image.usage = imageUsageSampled;

	log_message(VERBOSE, "Done computing room texture.");
}
