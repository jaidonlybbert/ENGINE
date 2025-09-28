#ifndef ENG_SWAPCHAIN
#define ENG_SWAPCHAIN
#include<vector>

#include<vulkan/vulkan.h>
#include<GLFW/glfw3.h>

#include "interfaces/PhysicalDevice.hpp"
#include "interfaces/Image.hpp"

namespace ENG
{

class Swapchain {
public:
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent; 
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	explicit Swapchain(const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface, const VkDevice &device, GLFWwindow &window);

	static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow &window);
	void cleanupSwapChain(const VkDevice &device);
	void createFramebuffers(const VkRenderPass &renderPass, const VkDevice &device);	
	void createSwapChain(const VkPhysicalDevice &physicalDevice, const VkSurfaceKHR &surface, const VkDevice &device, GLFWwindow &window);
	void recreateSwapChain(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR &surface, GLFWwindow* window, const VkRenderPass &renderPass);
};
}
#endif
