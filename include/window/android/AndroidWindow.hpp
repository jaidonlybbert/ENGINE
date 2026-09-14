#pragma once
#include <cstdint>

#include "window/WindowI.hpp"

struct android_app;

// The only WindowI implementation for Android (see issue #47). Backed by an
// android_native_app_glue android_app's current ANativeWindow*.
//
// Unlike GlfwWindow, this does NOT own the window's lifetime - android_native_app_glue
// creates/destroys the ANativeWindow itself in response to OS events (APP_CMD_INIT_WINDOW
// / APP_CMD_TERM_WINDOW), which can happen repeatedly across a single app run (e.g. every
// pause/resume - see issue #48's Application::notifySurfaceDestroyed()/
// notifySurfaceCreated(), which main_android.cpp wires up alongside this class). This
// class assumes app->window is already valid at construction time - the caller is
// responsible for pumping events until the first APP_CMD_INIT_WINDOW before constructing
// one (see main_android.cpp).
//
// Also unlike GlfwWindow, this does NOT register itself as app->onAppCmd directly - only
// one function can hold that slot at a time, and other things (Application's lifecycle
// notifications) need to observe the same event stream. Whoever owns app->onAppCmd is
// expected to call onAppCmd() below for every command, alongside anything else it needs
// to do with that same event.
class AndroidWindow : public WindowI {
   public:
    explicit AndroidWindow(android_app* app);
    ~AndroidWindow() override = default;

    AndroidWindow(const AndroidWindow&) = delete;
    AndroidWindow& operator=(const AndroidWindow&) = delete;

    // Non-blocking: processes whatever OS/input events are already pending and returns.
    void pollEvents() override;
    // Blocking: waits for at least one event, processes it, then returns.
    void waitEvents() override;
    bool shouldClose() const override;

    // Android has no separate screen-coordinate/framebuffer-pixel distinction the way
    // desktop high-DPI displays do - both query the same ANativeWindow dimensions.
    void getFramebufferSize(int& width, int& height) const override;
    void getWindowSize(int& width, int& height) const override;

    void setFramebufferResizeCallback(std::function<void()> callback) override;

    // Returns the current ANativeWindow*.
    void* nativeHandle() const override;

    // Only acts on APP_CMD_WINDOW_RESIZED (invoking the resize callback, if any) -
    // everything else is ignored here (surface destroy/create and pause/resume are
    // Application's concern, not WindowI's - see issue #48).
    void onAppCmd(int32_t cmd);

   private:
    android_app* app;
    std::function<void()> framebufferResizeCallback;
};
