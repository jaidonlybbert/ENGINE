// Android entry point (android_native_app_glue calls this instead of main()). This is
// intentionally a minimal stub for now - it only proves the Conan/NDK/CMake toolchain
// pipeline works end to end (see issue #46). Wiring up the actual engine (window/input/
// rendering/filesystem) is the follow-up issues #47-#51, each replacing one of the
// GLFW-specific pieces main.cpp uses on desktop with an Android equivalent.
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace {

void onAppCmd(android_app* app, int32_t cmd) {
    if (cmd == APP_CMD_INIT_WINDOW) {
        LOGI("Engine native library loaded and window is ready.");
    }
}

}  // namespace

void android_main(android_app* app) {
    app->onAppCmd = onAppCmd;

    int events;
    android_poll_source* source;
    while (!app->destroyRequested) {
        while (ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
        }
    }
}
