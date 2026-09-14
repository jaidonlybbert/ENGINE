#include "scenes/common/CameraControls.hpp"

#include <cmath>

#include "events/Event.hpp"
#include "logger/Logging.hpp"

namespace {

const TouchPoint* findTouchPointById(const std::vector<TouchPoint>& points, int id) {
    for (const auto& point : points) {
        if (point.id == id) {
            return &point;
        }
    }
    return nullptr;
}

double touchDistance(const TouchPoint& a, const TouchPoint& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

}  // namespace

void CameraControls::set_callbacks(WindowI& window, InputI& input, WindowUserData& windowUserData) {
    window.setFramebufferResizeCallback([&window, &windowUserData]() {
        // store screen size (in screen coordinates this is NOT the same as the framebuffer width and height)
        window.getWindowSize(windowUserData.windowWidthScreenCoords, windowUserData.windowHeightScreenCoords);
        windowUserData.windowResized = true;
    });

    input.addMouseScrollCallback([&windowUserData](double xoffset, double yoffset) {
        ENG_LOG_TRACE("Scroll Offset - X: " << xoffset << " Y: " << yoffset);

        static const auto invert_x = true;
        static const auto invert_y = false;
        static const auto sensitivity = static_cast<double>(1.f);

        xoffset *= sensitivity;
        yoffset *= sensitivity;
        xoffset = invert_x ? -xoffset : xoffset;
        yoffset = invert_y ? -yoffset : yoffset;

        ClientHidEvent hidEvent{};
        hidEvent.look_dx = xoffset;
        hidEvent.look_dy = yoffset;
        hidEvent.actions.push_back(Action::NODE_ROTATION_PRESERVE_Y_AS_UP);
        windowUserData.eventQueue.push_back(hidEvent);
    });

    input.addMouseMovementCallback([&input, &windowUserData](double xpos, double ypos) {
        if (input.isMouseButtonPressed(MouseButton::Middle)) {
            ENG_LOG_TRACE("Middle mouse down");
            const auto dx = xpos - windowUserData.cursorXScreenCoords;
            const auto dy = ypos - windowUserData.cursorYScreenCoords;

            ClientHidEvent hidEvent{};
            hidEvent.look_dx = dx;
            hidEvent.look_dy = dy;
            hidEvent.actions.push_back(Action::NODE_ROTATION_PRESERVE_Y_AS_UP);
            windowUserData.eventQueue.push_back(hidEvent);
        }
        // always update current position
        windowUserData.cursorXScreenCoords = xpos;
        windowUserData.cursorYScreenCoords = ypos;
    });

    input.addKeyCallback([](Key key, KeyAction action) {
        if (key == Key::T && action == KeyAction::Press) {
            ENG_LOG_TRACE("Toggle settings window visibility");
        }
        if (key == Key::E && action == KeyAction::Press) {
            ENG_LOG_TRACE("E key down");
        }
        if (key == Key::R && action == KeyAction::Press) {
            ENG_LOG_TRACE("R key down");
        }
    });

    input.addMouseButtonCallback([&input, &windowUserData](MouseButton button, KeyAction action) {
        if (button == MouseButton::Middle) {
            if (action == KeyAction::Press) {
                input.getCursorPosition(windowUserData.cursorXScreenCoords, windowUserData.cursorYScreenCoords);
                ENG_LOG_TRACE("Middle mouse initial press");
            } else {
                ENG_LOG_TRACE("Middle mouse released");
            }
        }
    });

    input.addTouchCallback([&windowUserData](TouchPhase phase, const std::vector<TouchPoint>& points) {
        if (phase == TouchPhase::Move) {
            if (points.size() == 1 && windowUserData.previousTouchPoints.size() == 1) {
                // Single-finger drag - the same orbit gesture as middle-mouse-drag.
                if (const auto* previous = findTouchPointById(windowUserData.previousTouchPoints, points[0].id)) {
                    ClientHidEvent hidEvent{};
                    hidEvent.look_dx = points[0].x - previous->x;
                    hidEvent.look_dy = points[0].y - previous->y;
                    hidEvent.actions.push_back(Action::NODE_ROTATION_PRESERVE_Y_AS_UP);
                    windowUserData.eventQueue.push_back(hidEvent);
                }
            } else if (points.size() == 2 && windowUserData.previousTouchPoints.size() == 2) {
                // Two-finger pinch - reuses the same rotation action scroll already does
                // (there's no separate "zoom" concept in this camera system), using the
                // change in inter-finger distance as the analog for scroll's dy.
                const auto* previousA = findTouchPointById(windowUserData.previousTouchPoints, points[0].id);
                const auto* previousB = findTouchPointById(windowUserData.previousTouchPoints, points[1].id);
                if (previousA != nullptr && previousB != nullptr) {
                    const auto previousDistance = touchDistance(*previousA, *previousB);
                    const auto currentDistance = touchDistance(points[0], points[1]);

                    ClientHidEvent hidEvent{};
                    hidEvent.look_dx = 0.0;
                    hidEvent.look_dy = currentDistance - previousDistance;
                    hidEvent.actions.push_back(Action::NODE_ROTATION_PRESERVE_Y_AS_UP);
                    windowUserData.eventQueue.push_back(hidEvent);
                }
            }
        }

        windowUserData.previousTouchPoints = points;
    });
}
