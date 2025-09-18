#include "vulkan/vulkan.h"
#include<vector>

#include "EngineConfig.h"

namespace ENG
{
class Command {
public:
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	explicit Command(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface);

	void createCommandPool(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface);	
	void createCommandBuffers(const VkDevice& device);
	VkCommandBuffer beginSingleTimeCommands(const VkDevice& device);
	void endSingleTimeCommands(const VkDevice& device, const VkQueue &graphicsQueue, VkCommandBuffer &commandBuffer);
};
}
