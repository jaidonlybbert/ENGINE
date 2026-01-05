#ifndef ENG_PHYSICAL_DEVICE
#define ENG_PHYSICAL_DEVICE
#include<vulkan/vulkan_core.h>
#include<vector>
#include<optional>

namespace ENG
{

static const std::vector<const char*> deviceExtensions {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef __APPLE__
		// Support for MoltenVK for macOS, disabled for KosmicKrisp
		"VK_KHR_portability_subset"
		// VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
#endif
};

struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete();
};

class PhysicalDevice {
public:
	VkPhysicalDevice physicalDevice;

	static bool checkDeviceExtensionSupport(VkPhysicalDevice device);
	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, const VkSurfaceKHR &surface);
	static SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice &device, const VkSurfaceKHR &surface);	
	static bool isDeviceSuitable(VkPhysicalDevice device, const VkSurfaceKHR &surface);
	static void pickPhysicalDevice(const VkInstance& instance, VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface);
};
}
#endif
