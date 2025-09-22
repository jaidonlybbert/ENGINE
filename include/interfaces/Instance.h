#ifndef ENG_INSTANCE
#define ENG_INSTANCE
#include<vector>
#include<GLFW/glfw3.h>
#include<vulkan/vulkan_core.h>

#include "EngineConfig.h"

#ifdef NDEBUG
	constexpr bool enableValidationLayers = false;
#else
	constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};


namespace ENG
{

class InstanceFactory
{
public:
	VkInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
			const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
			const VkAllocationCallbacks* pAllocator, 
			VkDebugUtilsMessengerEXT* pDebugMessenger);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
			VkDebugUtilsMessengerEXT debugMessenger, 
			const VkAllocationCallbacks* pAllocator);
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void setupDebugMessenger();
	void createInstance();
	std::vector<const char*> getRequiredExtensions();
	bool checkValidationLayerSupport();
};
} // end namespace
#endif
