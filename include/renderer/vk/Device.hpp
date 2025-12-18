#ifndef ENG_DEVICE
#define ENG_DEVICE
#include<vulkan/vulkan.h>
#include<vector>
#include<stdexcept>

#include "EngineConfig.hpp"

namespace ENG
{
class Device
{

public:
	static uint32_t findMemoryType(const VkPhysicalDevice &physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		    }
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	static void createLogicalDevice(const VkSurfaceKHR &surface, const VkPhysicalDevice &physicalDevice,
				 const std::vector<const char*> &validationLayers,
				 VkQueue &graphicsQueue, VkQueue &presentQueue, VkDevice &device);
};
}
#endif
