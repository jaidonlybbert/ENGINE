#pragma once

#include <deque>
#include <functional>

#include "events/Event.hpp"

namespace ENG {
class Node;
}

// Only the keys/buttons actually referenced anywhere in the engine today - add to these
// as new bindings are needed rather than mirroring GLFW's full key set up front.
enum class Key { LeftShift, T, E, R };
enum class MouseButton { Middle };
enum class KeyAction { Press, Release };

struct WindowUserData {
    double cursorXScreenCoords{0.};
    double cursorYScreenCoords{0.};
    int windowWidthScreenCoords{0};
    int windowHeightScreenCoords{0};
    std::deque<ClientHidEvent> eventQueue;
    bool windowResized{false};
};

// Abstracts keyboard/mouse input so callers (CameraControls, scenes) don't call into a
// specific windowing toolkit directly. GLFW (hid/glfw/GlfwInput.hpp) is the only
// implementation today - a future Android backend would report touch input through the
// same interface instead (see issue #42).
class InputI {
   public:
    virtual ~InputI() = default;

    virtual bool isKeyPressed(Key key) const = 0;
    virtual bool isMouseButtonPressed(MouseButton button) const = 0;
    virtual void getCursorPosition(double& x, double& y) const = 0;

    // Any number of listeners may register for each event category.
    virtual void addMouseScrollCallback(std::function<void(double xoffset, double yoffset)> callback) = 0;
    virtual void addMouseMovementCallback(std::function<void(double xpos, double ypos)> callback) = 0;
    virtual void addMouseButtonCallback(std::function<void(MouseButton button, KeyAction action)> callback) = 0;
    virtual void addKeyCallback(std::function<void(Key key, KeyAction action)> callback) = 0;
};

/*
 * Blender style camera - rotation around y axis is global, rotation around x axis is local to active object.
 * Reverses rotation around y-axis when camera is 'upside down'
 */
void node_rotation_follows_input_preserve_y_as_up(ENG::Node& activeNode, const double dx, const double dy);
void node_rotation_follows_input(ENG::Node& activeNode, const double dx, const double dy);
