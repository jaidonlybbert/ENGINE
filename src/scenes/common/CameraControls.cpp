#include "scenes/common/CameraControls.hpp"

#include "events/Event.hpp"
#include "logger/Logging.hpp"

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
}
