// Android entry point (android_native_app_glue calls this instead of main()). Exercises
// AndroidWindow and the Android branch of InstanceFactory::getRequiredExtensions() /
// ENG::createWindowSurface() end to end (see issue #47): creates a real VkInstance and
// VkSurfaceKHR from the window and logs whether it worked.
//
// Also wires Application's lifecycle hooks (issue #48) to the OS events that would
// destroy/recreate the surface (APP_CMD_TERM_WINDOW/APP_CMD_INIT_WINDOW) and
// pause/resume the app (APP_CMD_PAUSE/APP_CMD_RESUME), logging each one. This
// deliberately doesn't construct a real, persistent VkRenderer to exercise
// VkRenderer::handleSurfaceDestroyed()/handleSurfaceCreated() against - initVulkan() also
// picks a physical device, builds a swapchain, and preloads textures through
// AssetProviderI, none of which is Android-ready yet (issue #50, same blocker #47 hit).
// Those two methods are real and compile for Android (see Renderer.cpp), ready for
// whichever future issue can actually construct a persistent Android VkRenderer to call
// them from. What's exercised here is the Application register/notify plumbing itself,
// and that AndroidWindow correctly hands every OS command off to it.
//
// Also wires up AndroidInput and CameraControls::set_callbacks() (issue #49) - touch-drag
// orbit and pinch both go through the same ClientHidEvent queue mouse-drag/scroll already
// do, so there's no scene/renderer here to visibly confirm against yet (same #50 blocker),
// but the touch-event plumbing, gesture math, and CameraControls wiring are all real and
// exercised end to end down to the event queue.
#include <android/log.h>
#include <android_native_app_glue.h>

#include <stdexcept>

#include "application/Application.hpp"
#include "hid/android/AndroidInput.hpp"
#include "renderer/vk/Instance.hpp"
#include "scenes/common/CameraControls.hpp"
#include "window/android/AndroidWindow.hpp"

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

bool windowReady = false;

// Only used to detect the first APP_CMD_INIT_WINDOW, before an AndroidWindow exists to
// hand commands off to (see the real dispatcher set up in android_main() below, once the
// window is ready).
void onBootstrapAppCmd(android_app* app, int32_t cmd) {
    if (cmd == APP_CMD_INIT_WINDOW) {
        windowReady = true;
    }
}

void verifyVulkanSurfaceCreation(AndroidWindow& window) {
    ENG::InstanceFactory instanceFactory;
    try {
        instanceFactory.createInstance();
    } catch (const std::exception& e) {
        LOGE("Failed to create Vulkan instance: %s", e.what());
        return;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    if (ENG::createWindowSurface(instanceFactory.instance, window, &surface) == VK_SUCCESS) {
        LOGI("AndroidWindow created a Vulkan surface successfully.");
        vkDestroySurfaceKHR(instanceFactory.instance, surface, nullptr);
    } else {
        LOGE("AndroidWindow failed to create a Vulkan surface.");
    }

    vkDestroyInstance(instanceFactory.instance, nullptr);
}

// Everything the real (post-bootstrap) app->onAppCmd/app->onInputEvent dispatchers need,
// stashed in android_app::userData since both are plain C function pointers - no
// captures.
struct DispatchState {
    AndroidWindow* window;
    Application* application;
    AndroidInput* input;
};

void onAppCmd(android_app* app, int32_t cmd) {
    auto* state = static_cast<DispatchState*>(app->userData);
    state->window->onAppCmd(cmd);

    switch (cmd) {
        case APP_CMD_TERM_WINDOW:
            state->application->notifySurfaceDestroyed();
            break;
        case APP_CMD_INIT_WINDOW:
            state->application->notifySurfaceCreated();
            break;
        case APP_CMD_PAUSE:
            state->application->notifyPause();
            break;
        case APP_CMD_RESUME:
            state->application->notifyResume();
            break;
        default:
            break;
    }
}

int32_t onInputEvent(android_app* app, AInputEvent* event) {
    auto* state = static_cast<DispatchState*>(app->userData);
    return state->input->onInputEvent(event);
}

}  // namespace

void android_main(android_app* app) {
    app->onAppCmd = onBootstrapAppCmd;

    // app->window (and therefore AndroidWindow's precondition) isn't valid until the
    // first APP_CMD_INIT_WINDOW arrives.
    int events;
    android_poll_source* source;
    while (!windowReady && !app->destroyRequested) {
        if (ALooper_pollOnce(-1, nullptr, &events, reinterpret_cast<void**>(&source)) >= 0 && source != nullptr) {
            source->process(app, source);
        }
    }

    if (app->destroyRequested) {
        return;
    }

    AndroidWindow window(app);
    verifyVulkanSurfaceCreation(window);

    Application application;
    application.registerSurfaceDestroyedCallback([]() { LOGI("Surface destroyed"); });
    application.registerSurfaceCreatedCallback([]() { LOGI("Surface (re)created"); });
    application.registerPauseCallback([]() { LOGI("App paused"); });
    application.registerResumeCallback([]() { LOGI("App resumed"); });

    AndroidInput input;
    WindowUserData windowUserData;
    window.getWindowSize(windowUserData.windowWidthScreenCoords, windowUserData.windowHeightScreenCoords);
    CameraControls::set_callbacks(window, input, windowUserData);

    DispatchState dispatchState{&window, &application, &input};
    app->userData = &dispatchState;
    app->onAppCmd = onAppCmd;
    app->onInputEvent = onInputEvent;

    while (!window.shouldClose()) {
        window.pollEvents();
        // Draining eventQueue would normally feed a scene's active camera node (see
        // handleHIDEvents() in main.cpp) - no scene exists on Android yet (issue #50), so
        // this just confirms touch gestures are actually reaching the queue.
        if (!windowUserData.eventQueue.empty()) {
            LOGI("CameraControls queued a look event from touch input.");
            windowUserData.eventQueue.clear();
        }
    }
}
