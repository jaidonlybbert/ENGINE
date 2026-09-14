#pragma once
#include <functional>

// Abstracts platform window/surface lifecycle so the renderer doesn't call into a
// specific windowing toolkit directly. GLFW (window/glfw/GlfwWindow.hpp) is the only
// implementation today; GLFW has no Android backend at all, so a future Android/iOS port
// needs a second implementation of this interface rather than a rewrite of everything
// that currently reaches into GLFW (see issue #42).
//
// Deliberately has no Vulkan (or any other graphics API) in its signature - windowing and
// rendering are independent concerns, and a future non-Vulkan rendering backend should be
// able to use this interface without depending on Vulkan. Vulkan-specific surface/instance
// glue lives in the Vulkan renderer instead (see VkRenderer::createSurface(),
// InstanceFactory::getRequiredExtensions()), using nativeHandle() the same way
// imgui_impl_glfw already does.
class WindowI {
   public:
    virtual ~WindowI() = default;

    virtual void pollEvents() = 0;
    // Blocks until at least one event arrives. Used to stall swapchain recreation while
    // the window has a 0x0 framebuffer (e.g. minimized) instead of busy-spinning.
    virtual void waitEvents() = 0;
    virtual bool shouldClose() const = 0;

    // Pixel dimensions of the drawable surface - what the swapchain should be sized to.
    virtual void getFramebufferSize(int& width, int& height) const = 0;
    // Window dimensions in screen coordinates, which can differ from the framebuffer
    // size on high-DPI displays. Used for cursor-position math against the window
    // bounds, not for anything Vulkan-sized.
    virtual void getWindowSize(int& width, int& height) const = 0;

    // Invoked whenever the framebuffer is resized (including minimize/restore).
    virtual void setFramebufferResizeCallback(std::function<void()> callback) = 0;

    // Escape hatch for integrations that have no cross-platform equivalent yet and need
    // the concrete backend's native handle directly (imgui_impl_glfw, Vulkan surface
    // creation - see the class comment above). Returns whatever native handle type the
    // concrete backend owns (GLFWwindow* for GlfwWindow), or nullptr if the backend has
    // none. Callers that use this already know which concrete backend they're bridging to.
    virtual void* nativeHandle() const = 0;
};
