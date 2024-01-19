#ifndef VK_TEXTURE_H
#define VK_TEXTURE_H

#include <stdint.h>

#include <vulkan/vulkan.h>

#include "render/texture_info.h"
#include "render/stb/image_data.h"

#include "image.h"
#include "physical_device.h"
#include "queue.h"

// Related to `animation_t`, this struct contains real-time dynamic information on *instances* of this texture--
// 	AKA, this struct controls the texture state of a single particular model bearing 
// 	the corresponding texture in the scene.
typedef struct texture_animation_cycle_t {

	// The total number of frames in this animation cycle.
	uint32_t num_frames;

	// The number of frames played per second.
	uint32_t frames_per_second;

} texture_animation_cycle_t;

typedef struct texture_t {

	uint32_t num_images;
	VkImage *images;
	VkImageView *image_views;
	texture_animation_cycle_t *animation_cycles;

	uint32_t current_animation_cycle;
	uint32_t current_animation_frame;
	uint64_t last_frame_time;	// Time point of last frame, in milliseconds.

	VkFormat format;
	VkImageLayout layout;
	VkDeviceMemory memory;

	// The device with which this image was created.
	VkDevice device;

} texture_t;

void destroy_texture(texture_t texture);

#endif	// VK_TEXTURE_H
