#ifndef ENG_IMAGE
#define ENG_IMAGE
#include "vulkan/vulkan.h"
#include<stdexcept>
#include<vector>
#include "renderer/Device.hpp"

namespace ENG
{ 
void createImage(const VkDevice &device, const VkPhysicalDevice &physicalDevice, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, 
	  VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
VkImageView createImageView(const VkDevice& device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
void createImageViews(const VkDevice &device, const std::vector<VkImage>& images, const VkFormat &format, std::vector<VkImageView>& imageViews);
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const VkFormatFeatureFlags features, const VkPhysicalDevice &physicalDevice);
VkFormat findDepthFormat(const VkPhysicalDevice &physicalDevice);
void createDepthResources(const VkDevice &device, const VkPhysicalDevice &physicalDevice, const VkExtent2D &swapChainExtent,
			  VkImage &depthImage, VkDeviceMemory &depthImageMemory, VkImageView &depthImageView);
bool hasStencilComponent(VkFormat format);
} // end namespace
#endif
