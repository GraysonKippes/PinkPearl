#include "vulkan_instance.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "debug.h"
#include "glfw/GLFWManager.h"
#include "log/Logger.h"
#include "util/Allocation.h"

#define NUM_VALIDATION_LAYERS 1

static const uint32_t num_validation_layers = NUM_VALIDATION_LAYERS;

static const char *validation_layers[NUM_VALIDATION_LAYERS] = {
	"VK_LAYER_KHRONOS_validation"
};

/* -- Get Vulkan Extension Functions -- */

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkDebugUtilsMessengerEXT *pDebugMessenger) {
	PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks *pAllocator) {
	PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

/* -- Function Definitions -- */

bool check_validation_layer_support(uint32_t num_required_layers, const char *required_layers[]) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Checking validation layer support...");

	uint32_t num_available_layers = 0;
	vkEnumerateInstanceLayerProperties(&num_available_layers, nullptr);

	if (num_available_layers < num_required_layers) return false;
	if (num_available_layers == 0) return num_required_layers == 0;

	VkLayerProperties *available_layers = heapAlloc(num_available_layers, sizeof(VkLayerProperties));
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

	heapFree(available_layers);
	available_layers = nullptr;

	return true;
}

VulkanInstance create_vulkan_instance(void) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating Vulkan instance...");

	VulkanInstance vulkan_instance = { };

	// Query the extensions required by GLFW for Vulkan.
	uint32_t num_glfw_extensions = 0;
	const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&num_glfw_extensions);

	uint32_t num_extensions;
	char **extensions;

	// If debug mode is enabled, append the debug-utilities extension.
	if (debug_enabled) {

		num_extensions = num_glfw_extensions + 1;	// Include space for the debug extension name.
		extensions = heapAlloc(num_extensions, sizeof(char *));	// Use calloc to set each uninitialized pointer to nullptr.
		if (!extensions) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Creating Vulkan instance: failed to allocate array of extension names.");
			return vulkan_instance;
		}

		for (uint32_t i = 0; i < num_glfw_extensions; ++i) {
			size_t glfw_extension_name_length = strlen(glfw_extensions[i]) + 1;	// +1 to include the null-terminator.
			extensions[i] = heapAlloc(glfw_extension_name_length, sizeof(char));	// Allocate enough space for the GLFW extension name.
			if (!extensions[i]) {
				logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Creating Vulkan instance: failed to allocate extension name.");
				heapFree(extensions);
				return vulkan_instance;
			}
			strncpy(extensions[i], glfw_extensions[i], glfw_extension_name_length);
		}

		// Append the debug extension name.
		size_t debug_extension_name_length = strlen(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) + 1;
		extensions[num_extensions - 1] = heapAlloc(debug_extension_name_length, sizeof(char));
		if (!extensions[num_extensions - 1]) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Creating Vulkan instance: failed to allocate extension name.");
			heapFree(extensions);
			return vulkan_instance;
		}
		strncpy(extensions[num_extensions - 1], VK_EXT_DEBUG_UTILS_EXTENSION_NAME, debug_extension_name_length);
	} else {
		
		num_extensions = num_glfw_extensions;
		extensions = heapAlloc(num_extensions, sizeof(char *));	// Use calloc to set each uninitialized pointer to nullptr.
		if (!extensions) {
			logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Creating Vulkan instance: failed to allocate array of extension names.");
			return vulkan_instance;
		}

		for (uint32_t i = 0; i < num_glfw_extensions; ++i) {
			size_t glfw_extension_name_length = strlen(glfw_extensions[i]) + 1;	// +1 to include the null-terminator.
			extensions[i] = heapAlloc(glfw_extension_name_length, sizeof(char));	// Allocate enough space for the GLFW extension name.
			if (!extensions[i]) {
				logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Creating Vulkan instance: failed to allocate extension name.");
				heapFree(extensions);
				return vulkan_instance;
			}
			strncpy(extensions[i], glfw_extensions[i], glfw_extension_name_length);
		}
	}

	for (uint32_t i = 0; i < num_extensions; ++i) {
		logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Enabling Vulkan extension \"%s\".", extensions[i]);
	}

	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = nullptr,
		.pApplicationName = APP_NAME,	// TODO - link to project config
		.applicationVersion = VK_MAKE_API_VERSION(0, 0, 2, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_API_VERSION(0, 0, 0, 0),
		.apiVersion = VK_API_VERSION_1_3
	};
	
	VkInstanceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.pApplicationInfo = &applicationInfo,
		.enabledExtensionCount = num_extensions,
		.ppEnabledExtensionNames = (const char **)extensions
	};

	// If debug mode is enabled, enable the validation layers.
	if (debug_enabled) {

		check_validation_layer_support(num_validation_layers, validation_layers);
		create_info.enabledLayerCount = num_validation_layers;
		create_info.ppEnabledLayerNames = validation_layers;

		vulkan_instance.layer_names.num_strings = num_validation_layers;
		vulkan_instance.layer_names.strings = validation_layers;

		for (uint32_t i = 0; i < num_validation_layers; ++i) {
			logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Enabling validation layer \"%s\".", validation_layers[i]);
		}
	}
	else {

		create_info.enabledLayerCount = 0;
		create_info.ppEnabledLayerNames = nullptr;

		vulkan_instance.layer_names.num_strings = 0;
		vulkan_instance.layer_names.strings = nullptr;
	}

	VkResult result = vkCreateInstance(&create_info, nullptr, &vulkan_instance.handle);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_FATAL, "Vulkan instance creation failed. (Error code: %i)", result);
		return (VulkanInstance){ };
	}

	for (size_t i = 0; i < num_extensions; ++i) {
		heapFree(extensions[i]);
	}
	heapFree(extensions);

	return vulkan_instance;
}

void destroy_vulkan_instance(VulkanInstance vulkan_instance) {
	vkDestroyInstance(vulkan_instance.handle, nullptr);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
	void *pUserData) {

	(void)type;
	(void)pUserData;

	LogLevel log_level = LOG_LEVEL_VERBOSE;

	switch (severity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
		log_level = LOG_LEVEL_VERBOSE;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		log_level = LOG_LEVEL_INFO;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		log_level = LOG_LEVEL_WARNING;
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		log_level = LOG_LEVEL_ERROR;
		break;
	}

	logMsg(loggerVulkan, log_level, "Vulkan: %s", pCallbackData->pMessage);

	return VK_FALSE;
}

void setup_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT *messenger_ptr) {

	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating Vulkan debug messenger...");

	VkDebugUtilsMessengerCreateInfoEXT create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	create_info.pNext = nullptr;
	create_info.flags = 0;
	create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	create_info.pfnUserCallback = debug_callback;
	create_info.pUserData = nullptr;

	if (CreateDebugUtilsMessengerEXT(vulkan_instance, &create_info, nullptr, messenger_ptr) != VK_SUCCESS) {
		// TODO - error handling
	}
}

void destroy_debug_messenger(VkInstance vulkan_instance, VkDebugUtilsMessengerEXT messenger) {
	if (debug_enabled && messenger != VK_NULL_HANDLE) {
		DestroyDebugUtilsMessengerEXT(vulkan_instance, messenger, nullptr);
	}
}

WindowSurface createWindowSurface(const VulkanInstance vulkanInstance) {
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Creating window surface...");
	
	VkSurfaceKHR vkSurface;
	const VkResult result = glfwCreateWindowSurface(vulkanInstance.handle, getAppWindow(), nullptr, &vkSurface);
	if (result != VK_SUCCESS) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error creating window surface: surface creation failed (error code: %i).", result);
		return (WindowSurface){ .vkSurface = VK_NULL_HANDLE, .vkInstance = VK_NULL_HANDLE };
	}
	
	logMsg(loggerVulkan, LOG_LEVEL_VERBOSE, "Created window surface.");
	return (WindowSurface){ .vkSurface = vkSurface, .vkInstance = vulkanInstance.handle };
}

void deleteWindowSurface(WindowSurface *const pWindowSurface) {
	if (!pWindowSurface) {
		logMsg(loggerVulkan, LOG_LEVEL_ERROR, "Error deleting window surface: pointer to window surface object is null.");
	}
	
	vkDestroySurfaceKHR(pWindowSurface->vkInstance, pWindowSurface->vkSurface, nullptr);
	pWindowSurface->vkSurface = VK_NULL_HANDLE;
	pWindowSurface->vkInstance = VK_NULL_HANDLE;
}