#pragma once
#include "window/WindowI.hpp"

struct android_app;

// The only WindowI implementation for Android (see issue #47). Backed by an
// android_native_app_glue android_app's current ANativeWindow*.
//
// Unlike GlfwWindow, this does NOT own the window's lifetime - android_native_app_glue
// creates/destroys the ANativeWindow itself in response to OS events (APP_CMD_INIT_WINDOW
// / APP_CMD_TERM_WINDOW), which can happen repeatedly across a single app run (e.g. every
// pause/resume). This class assumes app->window is already valid at construction time -
// the caller is responsible for pumping events until the first APP_CMD_INIT_WINDOW before
// constructing one (see main_android.cpp) - and only takes over app->onAppCmd from that
// point forward, for framebuffer-resize tracking. Surviving a *later* window loss/
// recreation without tearing down and rebuilding the whole app is issue #48's scope
// (application lifecycle hooks), not this class's.
class AndroidWindow : public WindowI {
   public:
    explicit AndroidWindow(android_app* app);
    ~AndroidWindow() override;

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

   private:
    android_app* app;
    std::function<void()> framebufferResizeCallback;

    static void onAppCmdTrampoline(android_app* app, int32_t cmd);
    inline static AndroidWindow* activeInstance = nullptr;
};
