#ifndef SHADER_H
#define SHADER_H

#include <stdbool.h>

#include <vulkan/vulkan.h>

typedef struct shader_module_t {
	VkShaderModule module_handle;
	VkDevice device;
} shader_module_t;

shader_module_t create_shader_module(VkDevice device, const char *const filename);
bool destroy_shader_module(shader_module_t *const shader_module_ptr);

#endif	// SHADER_H
