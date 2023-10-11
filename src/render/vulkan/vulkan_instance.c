#include "vulkan_instance.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"
#include "glfw/glfw_manager.h"

static const uint32_t num_validation_layers = 1;

static const char *validation_layers[] = {
	"VK_LAYER_KHRONOS_validation"
};

/* -- Get Vulkan Extension Functions -- */

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != NULL)
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	else
		return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != NULL) {
		func(instance, debugMessenger, pAllocator);
	}
}

/* -- Function Definitions -- */

bool check_validation_layer_support(uint32_t num_required_layers, const char *required_layers[]) {

	uint32_t num_available_layers = 0;
	vkEnumerateInstanceLayerProperties(&num_available_layers, NULL);

	if (num_available_layers < num_required_layers)
		return false;

	if (num_available_layers == 0) {
		if (num_required_layers == 0)
			return true;
		else
			return false;
	}

	VkLayerProperties *available_layers = malloc(num_available_layers * sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&num_available_layers, available_layers);

	for (size_t i = 0; i < num_required_layers; ++i) {

		const char *layer_name = required_layers[i];
		bool layer_found = false;

		for (size_t j = 0; j < num_available_layers; ++j) {
			VkLayerProperties available_layer = available_layers[j];
			if (strcmp(layer_name, available_layer.layerName) == 0) {
				layer_found = true;
				break;
			}
		}

		if (!layer_found)
			return false;
	}

	free(available_layers);
	available_layers = NULL;

	return true;
}

void create_vulkan_instance(VkInstance *instance_ptr) {
	
	VkApplicationInfo app_info;
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Pink Pearl";	// TODO - link to project config
	app_info.applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_3;

	// Query the extensions required by GLFW for Vulkan.
	uint32_t num_glfw_extensions = 0;
	const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&num_glfw_extensions);

	uint32_t num_extensions;
	char **extensions;

	// If debug mode is enabled, append the debug-utilities extension.
	if (debug_enabled) {

		num_extensions = num_glfw_extensions + 1;	// Include space for the debug extension name.
		extensions = calloc(num_extensions, sizeof(char *));	// Use calloc to set each uninitialized pointer to NULL.

		for (uint32_t i = 0; i < num_glfw_extensions; ++i) {
			size_t glfw_extension_name_length = strlen(glfw_extensions[i]) + 1;	// +1 to include the null-terminator.
			*(extensions + i) = malloc(glfw_extension_name_length * sizeof(char));	// Allocate enough space for the GLFW extension name.
			strncpy(*(extensions + i), glfw_extensions[i], glfw_extension_name_length);
		}

		// Append the debug extension name.
		size_t debug_extension_name_length = strlen(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) + 1;
		*(extensions + num_extensions - 1) = malloc(debug_extension_name_length * sizeof(char));
		strncpy(*(extensions + num_extensions - 1), VK_EXT_DEBUG_UTILS_EXTENSION_NAME, debug_extension_name_length);
	}
	else {
		
		num_extensions = num_glfw_extensions;
		extensions = calloc(num_extensions, sizeof(char *));	// Use calloc to set each uninitialized pointer to NULL.

		for (uint32_t i = 0; i < num_glfw_extensions; ++i) {
			size_t glfw_extension_name_length = strlen(glfw_extensions[i]) + 1;	// +1 to include the null-terminator.
			*(extensions + i) = malloc(glfw_extension_name_length * sizeof(char));	// Allocate enough space for the GLFW extension name.
			strncpy(*(extensions + i), glfw_extensions[i], glfw_extension_name_length);
		}

	}

	VkInstanceCreateInfo create_info;
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = num_extensions;
	create_info.ppEnabledExtensionNames = (const char **)extensions;

	// If debug mode is enabled, enable the validation layers.
	if (debug_enabled) {
		check_validation_layer_support(num_validation_layers, validation_layers);
		create_info.enabledLayerCount = num_validation_layers;
		create_info.ppEnabledLayerNames = validation_layers;
	}
	else {
		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;
	}

	VkResult result = vkCreateInstance(&create_info, NULL, instance_ptr);
	if (result != VK_SUCCESS) {
		// TODO - error handling
	}

	for (size_t i = 0; i < num_extensions; ++i)
		free(extensions[i]);
	free(extensions);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData) {

	static const bool showVerbose = false;
	static const bool showInfo = false;
	static const bool showWarning = true;
	static const bool showError = true;

	// Console output formatting and labeling.
	char label[13];
	static const char *clear_console_format = "\x1B[39m";

	// Severity text color
	switch (severity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		if (!showVerbose) return VK_FALSE;
		strcpy(label, "\x1B[37mVerbose");	// Verbose - gray
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		if (!showInfo) return VK_FALSE;
		strcpy(label, "\x1B[97mInfo");		// Info - white
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		if (!showWarning) return VK_FALSE;
		strcpy(label, "\x1B[93mWarning");	// Warning - yellow
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		if (!showError) return VK_FALSE;
		strcpy(label, "\x1B[91mError");		// Error - red
		break;
	}

	fprintf(stderr, "%s: %s%s\n", label, pCallbackData->pMessage, clear_console_format);
	return VK_FALSE;
}

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr) {

	VkDebugUtilsMessengerCreateInfoEXT create_info;
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debugCallback;
	create_info.pUserData = NULL;

	if (CreateDebugUtilsMessengerEXT(vulkan_instance, &create_info, NULL, messenger_ptr) != VK_SUCCESS) {
		// TODO - error handling
	}
}

void destroy_vulkan_instance(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger) {
	if (debug_enabled)
		DestroyDebugUtilsMessengerEXT(vulkan_instance, messenger, NULL);
	vkDestroyInstance(vulkan_instance, NULL);
}
