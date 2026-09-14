#include "window/android/AndroidWindow.hpp"

#include <android/native_window.h>
#include <android_native_app_glue.h>

#include <cassert>

AndroidWindow::AndroidWindow(android_app* app) : app(app) {
    assert(activeInstance == nullptr && "only one AndroidWindow may exist at a time");
    assert(app->window != nullptr &&
           "AndroidWindow requires an already-valid ANativeWindow - wait for APP_CMD_INIT_WINDOW first");
    activeInstance = this;
    app->onAppCmd = onAppCmdTrampoline;
}

AndroidWindow::~AndroidWindow() { activeInstance = nullptr; }

void AndroidWindow::pollEvents() {
    int events;
    android_poll_source* source;
    while (ALooper_pollOnce(0, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
        if (source != nullptr) {
            source->process(app, source);
        }
    }
}

void AndroidWindow::waitEvents() {
    int events;
    android_poll_source* source;
    if (ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0 && source != nullptr) {
        source->process(app, source);
    }
}

bool AndroidWindow::shouldClose() const { return app->destroyRequested != 0; }

void AndroidWindow::getFramebufferSize(int& width, int& height) const {
    width = ANativeWindow_getWidth(app->window);
    height = ANativeWindow_getHeight(app->window);
}

void AndroidWindow::getWindowSize(int& width, int& height) const { getFramebufferSize(width, height); }

void AndroidWindow::setFramebufferResizeCallback(std::function<void()> callback) {
    framebufferResizeCallback = std::move(callback);
}

void* AndroidWindow::nativeHandle() const { return app->window; }

void AndroidWindow::onAppCmdTrampoline(android_app* app, int32_t cmd) {
    if (cmd == APP_CMD_WINDOW_RESIZED && activeInstance && activeInstance->framebufferResizeCallback) {
        activeInstance->framebufferResizeCallback();
    }
}
