#ifndef SHADER_H
#define SHADER_H

/* PLANNED SHADERS
 *
 * compute_matrices - creates and multiplies a matrix for each renderable entity on screen.
 * compute_vertices - generates vertices for models, particularly for tilesets.
*/

#include <vulkan/vulkan.h>

void create_shader_module(VkDevice device, const char *filename, VkShaderModule *shader_module_ptr);

#endif	// SHADER_H
