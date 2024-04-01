#include "vulkan_instance.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <GLFW/glfw3.h>

#include "config.h"
#include "debug.h"
#include "log/logging.h"

#define NUM_VALIDATION_LAYERS 1

static const uint32_t num_validation_layers = NUM_VALIDATION_LAYERS;

static const char *validation_layers[NUM_VALIDATION_LAYERS] = {
	"VK_LAYER_KHRONOS_validation"
};

/* -- Get Vulkan Extension Functions -- */

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != NULL) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != NULL) {
		func(instance, debugMessenger, pAllocator);
	}
}

/* -- Function Definitions -- */

bool check_validation_layer_support(uint32_t num_required_layers, const char *required_layers[]) {

	log_message(VERBOSE, "Checking validation layer support...");

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

	VkLayerProperties *available_layers = calloc(num_available_layers, sizeof(VkLayerProperties));
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

vulkan_instance_t create_vulkan_instance(void) {

	log_message(VERBOSE, "Creating Vulkan instance...");

	vulkan_instance_t vulkan_instance = { 0 };

	VkApplicationInfo app_info = {0};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext = NULL;
	app_info.pApplicationName = APP_NAME;	// TODO - link to project config
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
		*(extensions + num_extensions - 1) = calloc(debug_extension_name_length, sizeof(char));
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

	for (uint32_t i = 0; i < num_extensions; ++i) {
		logf_message(VERBOSE, "Enabling Vulkan extension \"%s\".", extensions[i]);
	}

	VkInstanceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.pApplicationInfo = &app_info;
	create_info.enabledExtensionCount = num_extensions;
	create_info.ppEnabledExtensionNames = (const char **)extensions;

	// If debug mode is enabled, enable the validation layers.
	if (debug_enabled) {

		check_validation_layer_support(num_validation_layers, validation_layers);
		create_info.enabledLayerCount = num_validation_layers;
		create_info.ppEnabledLayerNames = validation_layers;

		vulkan_instance.layer_names.num_strings = num_validation_layers;
		vulkan_instance.layer_names.strings = validation_layers;

		for (uint32_t i = 0; i < num_validation_layers; ++i) {
			logf_message(VERBOSE, "Enabling validation layer \"%s\".", validation_layers[i]);
		}
	}
	else {

		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = NULL;

		vulkan_instance.layer_names.num_strings = 0;
		vulkan_instance.layer_names.strings = NULL;
	}

	VkResult result = vkCreateInstance(&create_info, NULL, &vulkan_instance.handle);
	if (result != VK_SUCCESS) {
		logf_message(FATAL, "Vulkan instance creation failed. (Error code: %i)", result);
		exit(1);
	}

	for (size_t i = 0; i < num_extensions; ++i) {
		free(extensions[i]);
	}
	free(extensions);

	return vulkan_instance;
}

void destroy_vulkan_instance(vulkan_instance_t vulkan_instance) {
	vkDestroyInstance(vulkan_instance.handle, NULL);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData) {

	(void)type;
	(void)pUserData;

	log_level_t log_level = VERBOSE;

	switch (severity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		log_level = VERBOSE;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		log_level = INFO;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		log_level = WARNING;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		log_level = ERROR;
		break;
	}

	logf_message(log_level, "Vulkan: %s", pCallbackData->pMessage);

	return VK_FALSE;
}

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr) {

	log_message(VERBOSE, "Creating Vulkan debug messenger...");

	VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.pNext = NULL;
	create_info.flags = 0;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_callback;
	create_info.pUserData = NULL;

	if (CreateDebugUtilsMessengerEXT(vulkan_instance, &create_info, NULL, messenger_ptr) != VK_SUCCESS) {
		// TODO - error handling
	}
}

void destroy_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger) {
	if (debug_enabled && messenger != VK_NULL_HANDLE) {
		DestroyDebugUtilsMessengerEXT(vulkan_instance, messenger, NULL);
	}
}
