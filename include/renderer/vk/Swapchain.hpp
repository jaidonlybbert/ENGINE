#ifndef ENG_SWAPCHAIN
#define ENG_SWAPCHAIN
#include <vulkan/vulkan.h>

#include <vector>

#include "window/WindowI.hpp"

namespace ENG {

class Swapchain {
   public:
    VkSwapchainKHR swapChain{VK_NULL_HANDLE};
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkImage depthImage{VK_NULL_HANDLE};
    VkDeviceMemory depthImageMemory{VK_NULL_HANDLE};
    VkImageView depthImageView{VK_NULL_HANDLE};

    explicit Swapchain(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, const VkDevice& device,
                       WindowI& window);

    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, WindowI& window);
    void cleanupSwapChain(const VkDevice& device);
    void createFramebuffers(const VkRenderPass& renderPass, const VkDevice& device);
    void createSwapChain(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, const VkDevice& device,
                         WindowI& window);
    // Returns false, leaving swapChain == VK_NULL_HANDLE and no framebuffers, if the surface
    // can't host a swapchain right now (Android, mid-teardown of the window) - the caller
    // should skip drawing and try again once the surface is valid.
    bool recreateSwapChain(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface,
                           WindowI& window, const VkRenderPass& renderPass);
};
}  // namespace ENG
#endif
