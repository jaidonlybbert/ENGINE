#include<limits>
#include<algorithm>
#include<array>
#include "interfaces/Swapchain.hpp"
#include "Image.hpp"

namespace ENG
{

Swapchain::Swapchain(const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface, const VkDevice &device, GLFWwindow &window)
{
	createSwapChain(physicalDevice, surface, device, window);
	createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);
}

VkSurfaceFormatKHR Swapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR Swapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Swapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow &window) {
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	} 

	int width, height;
	glfwGetFramebufferSize(&window, &width, &height);

	VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
	};

	actualExtent.width = std::clamp(actualExtent.width, 
			capabilities.minImageExtent.width,
			capabilities.maxImageExtent.width);
	actualExtent.height = std::clamp(actualExtent.height,
			capabilities.minImageExtent.height,
			capabilities.maxImageExtent.height);

	return actualExtent;
}

void Swapchain::createSwapChain(const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface, const VkDevice &device, GLFWwindow &window) {
	SwapChainSupportDetails swapChainSupport = ENG::PhysicalDevice::querySwapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = ENG::PhysicalDevice::findQueueFamilies(physicalDevice, surface);
	uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
		indices.presentFamily.value()};

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	} else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
			swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Swapchain::createFramebuffers(const VkRenderPass &renderPass, const VkDevice &device) {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			swapChainImageViews[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

	    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
		throw std::runtime_error("failed to create framebuffer!");
	    }
	}
}

void Swapchain::cleanupSwapChain(const VkDevice &device) {
	vkDestroyImageView(device, depthImageView, nullptr);
	vkDestroyImage(device, depthImage, nullptr);
	vkFreeMemory(device, depthImageMemory, nullptr);

	for (auto framebuffer : swapChainFramebuffers) {
		vkDestroyFramebuffer(device, framebuffer, nullptr);
	}

	for (auto imageView : swapChainImageViews) {
		vkDestroyImageView(device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void Swapchain::recreateSwapChain(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR &surface, GLFWwindow* window, const VkRenderPass &renderPass) {
	int width = 0, height = 0;
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}

	vkDeviceWaitIdle(device);

	cleanupSwapChain(device);

	createSwapChain(physicalDevice, surface, device, *window);
	createImageViews(device, swapChainImages, swapChainImageFormat, swapChainImageViews);
	createDepthResources(device, physicalDevice, swapChainExtent,
		  depthImage, depthImageMemory, depthImageView);
	createFramebuffers(renderPass, device);
}

} // end namespace
