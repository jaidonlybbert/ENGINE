#pragma once
#include <string>

#include "window/WindowI.hpp"

struct GLFWwindow;

// The only WindowI implementation today (desktop: Windows/Linux/macOS via GLFW). Owns
// the GLFWwindow's lifetime - constructing one calls glfwInit()/glfwCreateWindow(),
// destroying it calls glfwDestroyWindow()/glfwTerminate().
//
// The engine only ever has one window/GlfwWindow instance at a time, so GLFW callback
// trampolines dispatch through a single static instance pointer rather than GLFW's
// per-window user-pointer slot - that slot is also used by GlfwInput (hid/glfw/
// GlfwInput.hpp) for its own callbacks, and GLFW only allows one user pointer per window.
class GlfwWindow : public WindowI {
   public:
    GlfwWindow(int width, int height, const std::string& title);
    ~GlfwWindow() override;

    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    void pollEvents() override;
    void waitEvents() override;
    bool shouldClose() const override;

    void getFramebufferSize(int& width, int& height) const override;
    void getWindowSize(int& width, int& height) const override;

    std::vector<const char*> getRequiredInstanceExtensions() const override;
    VkResult createSurface(VkInstance instance, VkSurfaceKHR* surface) const override;

    void setFramebufferResizeCallback(std::function<void()> callback) override;

    void* nativeHandle() const override;

   private:
    GLFWwindow* window;
    std::function<void()> framebufferResizeCallback;

    static void framebufferSizeCallbackTrampoline(GLFWwindow* window, int width, int height);
    inline static GlfwWindow* activeInstance = nullptr;
};
