#include "hid/android/AndroidInput.hpp"

#include <android/input.h>

namespace {

TouchPhase toTouchPhase(int32_t action) {
    switch (action & AMOTION_EVENT_ACTION_MASK) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            return TouchPhase::Down;
        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            return TouchPhase::Up;
        case AMOTION_EVENT_ACTION_CANCEL:
            return TouchPhase::Cancel;
        default:
            return TouchPhase::Move;
    }
}

}  // namespace

bool AndroidInput::isKeyPressed(Key key) const { return false; }

bool AndroidInput::isMouseButtonPressed(MouseButton button) const { return false; }

void AndroidInput::getCursorPosition(double& x, double& y) const {
    x = 0.0;
    y = 0.0;
}

void AndroidInput::addMouseScrollCallback(std::function<void(double, double)> callback) {}

void AndroidInput::addMouseMovementCallback(std::function<void(double, double)> callback) {}

void AndroidInput::addMouseButtonCallback(std::function<void(MouseButton, KeyAction)> callback) {}

void AndroidInput::addKeyCallback(std::function<void(Key, KeyAction)> callback) {}

void AndroidInput::addTouchCallback(std::function<void(TouchPhase, const std::vector<TouchPoint>&)> callback) {
    touchCallbacks.push_back(std::move(callback));
}

int32_t AndroidInput::onInputEvent(AInputEvent* event) {
    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) {
        return 0;
    }

    const int32_t action = AMotionEvent_getAction(event);
    const TouchPhase phase = toTouchPhase(action);

    std::vector<TouchPoint> points;
    const size_t pointerCount = AMotionEvent_getPointerCount(event);
    points.reserve(pointerCount);
    for (size_t i = 0; i < pointerCount; ++i) {
        points.push_back(
            TouchPoint{AMotionEvent_getPointerId(event, i), AMotionEvent_getX(event, i), AMotionEvent_getY(event, i)});
    }

    // AMOTION_EVENT_ACTION_*_UP events still report the lifted pointer in
    // AMotionEvent_getPointerCount() - drop it so callbacks see the truly *active* set,
    // matching InputI::addTouchCallback()'s documented contract.
    if (phase == TouchPhase::Up || phase == TouchPhase::Cancel) {
        const size_t liftedIndex = static_cast<size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                                                       AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
        if (liftedIndex < points.size()) {
            points.erase(points.begin() + liftedIndex);
        }
    }

    for (auto& callback : touchCallbacks) {
        callback(phase, points);
    }

    return 1;
}
