#include "hid/glfw/GlfwInput.hpp"

#include <GLFW/glfw3.h>

#include <cassert>

#include "window/glfw/GlfwWindow.hpp"

namespace {

Key toEngineKey(int glfwKey) {
    switch (glfwKey) {
        case GLFW_KEY_LEFT_SHIFT:
            return Key::LeftShift;
        case GLFW_KEY_T:
            return Key::T;
        case GLFW_KEY_E:
            return Key::E;
        case GLFW_KEY_R:
            return Key::R;
        default:
            return Key::LeftShift;  // unreached in practice - see the note on Key's enumerators
    }
}

int toGlfwKey(Key key) {
    switch (key) {
        case Key::LeftShift:
            return GLFW_KEY_LEFT_SHIFT;
        case Key::T:
            return GLFW_KEY_T;
        case Key::E:
            return GLFW_KEY_E;
        case Key::R:
            return GLFW_KEY_R;
    }
    return GLFW_KEY_UNKNOWN;
}

int toGlfwMouseButton(MouseButton button) {
    switch (button) {
        case MouseButton::Middle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
    }
    return GLFW_MOUSE_BUTTON_MIDDLE;
}

KeyAction toEngineKeyAction(int glfwAction) {
    return glfwAction == GLFW_RELEASE ? KeyAction::Release : KeyAction::Press;
}

}  // namespace

GlfwInput::GlfwInput(const GlfwWindow& glfwWindow) : window(static_cast<GLFWwindow*>(glfwWindow.nativeHandle())) {
    assert(activeInstance == nullptr && "only one GlfwInput may exist at a time");
    activeInstance = this;

    glfwSetScrollCallback(window, scrollCallbackTrampoline);
    glfwSetCursorPosCallback(window, cursorPosCallbackTrampoline);
    glfwSetMouseButtonCallback(window, mouseButtonCallbackTrampoline);
    glfwSetKeyCallback(window, keyCallbackTrampoline);
}

bool GlfwInput::isKeyPressed(Key key) const { return glfwGetKey(window, toGlfwKey(key)) == GLFW_PRESS; }

bool GlfwInput::isMouseButtonPressed(MouseButton button) const {
    return glfwGetMouseButton(window, toGlfwMouseButton(button)) == GLFW_PRESS;
}

void GlfwInput::getCursorPosition(double& x, double& y) const { glfwGetCursorPos(window, &x, &y); }

void GlfwInput::addMouseScrollCallback(std::function<void(double, double)> callback) {
    mouseScrollCallbacks.push_back(std::move(callback));
}

void GlfwInput::addMouseMovementCallback(std::function<void(double, double)> callback) {
    mouseMovementCallbacks.push_back(std::move(callback));
}

void GlfwInput::addMouseButtonCallback(std::function<void(MouseButton, KeyAction)> callback) {
    mouseButtonCallbacks.push_back(std::move(callback));
}

void GlfwInput::addKeyCallback(std::function<void(Key, KeyAction)> callback) {
    keyCallbacks.push_back(std::move(callback));
}

void GlfwInput::scrollCallbackTrampoline(GLFWwindow* window, double xoffset, double yoffset) {
    if (!activeInstance) return;
    for (auto& callback : activeInstance->mouseScrollCallbacks) {
        callback(xoffset, yoffset);
    }
}

void GlfwInput::cursorPosCallbackTrampoline(GLFWwindow* window, double xpos, double ypos) {
    if (!activeInstance) return;
    for (auto& callback : activeInstance->mouseMovementCallbacks) {
        callback(xpos, ypos);
    }
}

void GlfwInput::mouseButtonCallbackTrampoline(GLFWwindow* window, int button, int action, int mods) {
    if (!activeInstance) return;
    if (button != GLFW_MOUSE_BUTTON_MIDDLE) return;  // the only button anything currently listens for
    for (auto& callback : activeInstance->mouseButtonCallbacks) {
        callback(MouseButton::Middle, toEngineKeyAction(action));
    }
}

void GlfwInput::keyCallbackTrampoline(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!activeInstance) return;
    if (key != GLFW_KEY_LEFT_SHIFT && key != GLFW_KEY_T && key != GLFW_KEY_E && key != GLFW_KEY_R) {
        return;  // no callback distinguishes any other key today
    }
    for (auto& callback : activeInstance->keyCallbacks) {
        callback(toEngineKey(key), toEngineKeyAction(action));
    }
}
