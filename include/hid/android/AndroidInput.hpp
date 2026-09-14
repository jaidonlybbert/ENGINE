#pragma once
#include <cstdint>

#include "hid/InputI.hpp"

struct AInputEvent;

// The Android InputI implementation (see issue #49). Only reports touch, via
// addTouchCallback() - the desktop-shaped methods (isKeyPressed/isMouseButtonPressed/
// getCursorPosition/addMouseScrollCallback/addMouseMovementCallback/
// addMouseButtonCallback/addKeyCallback) are no-ops, since Android has no keyboard/mouse
// by default.
//
// Like AndroidWindow, this does not register itself as app->onInputEvent directly - only
// one function can hold that slot at a time. Whoever owns it (see main_android.cpp) is
// expected to call onInputEvent() below for every input event.
class AndroidInput : public InputI {
   public:
    AndroidInput() = default;

    bool isKeyPressed(Key key) const override;
    bool isMouseButtonPressed(MouseButton button) const override;
    void getCursorPosition(double& x, double& y) const override;

    void addMouseScrollCallback(std::function<void(double, double)> callback) override;
    void addMouseMovementCallback(std::function<void(double, double)> callback) override;
    void addMouseButtonCallback(std::function<void(MouseButton, KeyAction)> callback) override;
    void addKeyCallback(std::function<void(Key, KeyAction)> callback) override;

    void addTouchCallback(std::function<void(TouchPhase, const std::vector<TouchPoint>&)> callback) override;

    // Returns 1 if this was a touch event (matching android_native_app_glue's
    // onInputEvent convention for "handled"), 0 otherwise.
    int32_t onInputEvent(AInputEvent* event);

   private:
    std::vector<std::function<void(TouchPhase, const std::vector<TouchPoint>&)>> touchCallbacks;
};
