#ifndef ENG_COMMAND
#define ENG_COMMAND
#include "vulkan/vulkan_core.h"
#include<vector>

#include "EngineConfig.hpp"

namespace ENG
{
class Command {
public:
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;
	const VkDevice& device;
	const VkPhysicalDevice& physicalDevice;

	explicit Command(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface);
	~Command();

	void createCommandPool(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface);	
	void createCommandBuffers(const VkDevice& device);
	VkCommandBuffer beginSingleTimeCommands(const VkDevice& device);
	void endSingleTimeCommands(const VkDevice& device, const VkQueue &graphicsQueue, VkCommandBuffer &commandBuffer);
	void copyBuffer(const VkQueue &graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void copyBufferToImage(const VkQueue &graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void transitionImageLayout(const VkQueue &graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
};
}
#endif
