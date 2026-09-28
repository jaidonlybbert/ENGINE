#ifndef ENG_SWAPCHAIN
#define ENG_SWAPCHAIN
#include <vulkan/vulkan.h>

#include <glm/glm.hpp>
#include <vector>

#include "window/WindowI.hpp"

namespace ENG {

class Swapchain {
   public:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    // The size of the swapchain images - which, when preTransform below is a 90/270 degree
    // rotation, is the *pre-rotated* size, i.e. swapped relative to how the user sees the
    // screen (issue #61). Everything that renders into the swapchain (viewport, scissor,
    // framebuffers, depth) uses this; anything reasoning about the screen as the user sees
    // it (aspect ratio, UI layout) wants displayExtent() instead.
    VkExtent2D swapChainExtent;
    // What the swapchain was created with. Deliberately currentTransform, not IDENTITY:
    // it tells the presentation engine "these images are already rotated to match", so it
    // applies no rotation of its own - the renderer has to do it (see preRotationMatrix()
    // and rotateImGuiDrawData()). Always IDENTITY on desktop.
    VkSurfaceTransformFlagBitsKHR preTransform{VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR};
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkImage depthImage;
    VkDeviceMemory depthImageMemory;
    VkImageView depthImageView;

    explicit Swapchain(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, const VkDevice& device,
                       WindowI& window);

    static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, WindowI& window);

    static bool swapsAxes(VkSurfaceTransformFlagBitsKHR transform);
    // swapChainExtent as the user sees the screen (width/height swapped back for 90/270).
    VkExtent2D displayExtent() const;
    // Clip-space rotation to multiply onto the projection matrix so the scene comes out
    // upright once the presentation engine displays a pre-rotated image. Identity when
    // preTransform is identity.
    glm::mat4 preRotationMatrix() const;
    void cleanupSwapChain(const VkDevice& device);
    void createFramebuffers(const VkRenderPass& renderPass, const VkDevice& device);
    void createSwapChain(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface, const VkDevice& device,
                         WindowI& window);
    void recreateSwapChain(const VkPhysicalDevice& physicalDevice, const VkDevice& device, const VkSurfaceKHR& surface,
                           WindowI& window, const VkRenderPass& renderPass);
};
}  // namespace ENG
#endif
