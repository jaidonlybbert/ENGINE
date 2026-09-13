#pragma once
#include "hid/InputI.hpp"
#include "window/WindowI.hpp"

// Generic mouse/keyboard camera controls (orbit, zoom, framebuffer resize). Not tied to
// any particular scene's contents - any scene can register these callbacks.
struct CameraControls {
    static void set_callbacks(WindowI& window, InputI& input, WindowUserData& windowUserData);
};
