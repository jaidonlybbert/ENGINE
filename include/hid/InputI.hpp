#pragma once

#include <deque>
#include <functional>
#include <vector>

#include "events/Event.hpp"

namespace ENG {
class Node;
}

// Only the keys/buttons actually referenced anywhere in the engine today - add to these
// as new bindings are needed rather than mirroring GLFW's full key set up front.
enum class Key { LeftShift, T, E, R };
enum class MouseButton { Middle };
enum class KeyAction { Press, Release };

// What just happened to the touch point set - the points a Down/Move/Up/Cancel callback
// reports are the *current* active set after that change (e.g. an Up event's points no
// longer include the finger that was lifted).
enum class TouchPhase { Down, Move, Up, Cancel };

// A single active finger. id is stable for the duration of that finger's contact with the
// screen (Android's MotionEvent pointer id) - use it to track "the same finger" across
// consecutive callbacks even as other fingers come and go, e.g. for pinch gestures.
struct TouchPoint {
    int id;
    double x;
    double y;
};

struct WindowUserData {
    double cursorXScreenCoords{0.};
    double cursorYScreenCoords{0.};
    int windowWidthScreenCoords{0};
    int windowHeightScreenCoords{0};
    std::deque<ClientHidEvent> eventQueue;
    bool windowResized{false};
    // The touch set as of the last touch callback - lets a touch-move handler compute a
    // delta against each finger's previous position, the same way cursorXScreenCoords/
    // cursorYScreenCoords do for the mouse.
    std::vector<TouchPoint> previousTouchPoints;
};

// Abstracts keyboard/mouse/touch input so callers (CameraControls, scenes) don't call
// into a specific windowing toolkit directly. GLFW (hid/glfw/GlfwInput.hpp) and Android
// (hid/android/AndroidInput.hpp) are the two implementations today (see issues #42/#49).
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

    // Touch input (Android; GLFW has no touch, so GlfwInput registers no callbacks and
    // this never fires there). Fires whenever the active touch set changes - a finger
    // going down, moving, or lifting - with every currently-active point, not just the
    // one that changed, since gestures like pinch need to see all points at once.
    virtual void addTouchCallback(
        std::function<void(TouchPhase phase, const std::vector<TouchPoint>& points)> callback) = 0;
};

/*
 * Blender style camera - rotation around y axis is global, rotation around x axis is local to active object.
 * Reverses rotation around y-axis when camera is 'upside down'
 */
void node_rotation_follows_input_preserve_y_as_up(ENG::Node& activeNode, const double dx, const double dy);
void node_rotation_follows_input(ENG::Node& activeNode, const double dx, const double dy);
