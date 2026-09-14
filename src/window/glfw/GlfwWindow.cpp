#include "window/glfw/GlfwWindow.hpp"

#include <GLFW/glfw3.h>

#include <cassert>
#include <stdexcept>

GlfwWindow::GlfwWindow(int width, int height, const std::string& title) {
    assert(activeInstance == nullptr && "only one GlfwWindow may exist at a time");
    activeInstance = this;

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallbackTrampoline);
}

GlfwWindow::~GlfwWindow() {
    glfwDestroyWindow(window);
    glfwTerminate();
    activeInstance = nullptr;
}

void GlfwWindow::pollEvents() { glfwPollEvents(); }

void GlfwWindow::waitEvents() { glfwWaitEvents(); }

bool GlfwWindow::shouldClose() const { return glfwWindowShouldClose(window); }

void GlfwWindow::getFramebufferSize(int& width, int& height) const { glfwGetFramebufferSize(window, &width, &height); }

void GlfwWindow::getWindowSize(int& width, int& height) const { glfwGetWindowSize(window, &width, &height); }

void GlfwWindow::setFramebufferResizeCallback(std::function<void()> callback) {
    framebufferResizeCallback = std::move(callback);
}

void* GlfwWindow::nativeHandle() const { return window; }

void GlfwWindow::framebufferSizeCallbackTrampoline(GLFWwindow* window, int width, int height) {
    if (activeInstance && activeInstance->framebufferResizeCallback) {
        activeInstance->framebufferResizeCallback();
    }
}
