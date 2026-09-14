#pragma once
#include <vector>

#include "hid/InputI.hpp"

class GlfwWindow;
struct GLFWwindow;

// The only InputI implementation today. Registers itself as the raw GLFW input callbacks
// on construction and fans each event out to whatever's been added via InputI's
// add*Callback methods, translating GLFW's key/button codes to the engine-native
// Key/MouseButton enums along the way.
//
// Like GlfwWindow, this dispatches through a single static instance pointer rather than
// GLFW's per-window user-pointer slot, since GlfwWindow already uses that slot for its
// own framebuffer-resize callback and GLFW only allows one user pointer per window.
class GlfwInput : public InputI {
   public:
    explicit GlfwInput(const GlfwWindow& window);

    bool isKeyPressed(Key key) const override;
    bool isMouseButtonPressed(MouseButton button) const override;
    void getCursorPosition(double& x, double& y) const override;

    void addMouseScrollCallback(std::function<void(double, double)> callback) override;
    void addMouseMovementCallback(std::function<void(double, double)> callback) override;
    void addMouseButtonCallback(std::function<void(MouseButton, KeyAction)> callback) override;
    void addKeyCallback(std::function<void(Key, KeyAction)> callback) override;
    // GLFW has no touch input - this is a deliberate no-op (the callback is never
    // invoked, so there's nothing useful to store it for).
    void addTouchCallback(std::function<void(TouchPhase, const std::vector<TouchPoint>&)> callback) override;

   private:
    GLFWwindow* window;

    std::vector<std::function<void(double, double)>> mouseScrollCallbacks;
    std::vector<std::function<void(double, double)>> mouseMovementCallbacks;
    std::vector<std::function<void(MouseButton, KeyAction)>> mouseButtonCallbacks;
    std::vector<std::function<void(Key, KeyAction)>> keyCallbacks;

    static void scrollCallbackTrampoline(GLFWwindow* window, double xoffset, double yoffset);
    static void cursorPosCallbackTrampoline(GLFWwindow* window, double xpos, double ypos);
    static void mouseButtonCallbackTrampoline(GLFWwindow* window, int button, int action, int mods);
    static void keyCallbackTrampoline(GLFWwindow* window, int key, int scancode, int action, int mods);
    inline static GlfwInput* activeInstance = nullptr;
};
