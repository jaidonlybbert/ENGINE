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
// picks a physical device and builds a swapchain, and there's no build step that packages
// a scene's assets into an APK at all yet (see below), so a full render loop isn't
// Android-ready as a whole even though its individual pieces (this file's other
// verify*() functions) are. Those two methods are real and compile for Android (see
// Renderer.cpp), ready for whichever future issue can actually construct a persistent
// Android VkRenderer to call them from. What's exercised here is the Application
// register/notify plumbing itself, and that AndroidWindow correctly hands every OS command
// off to it.
//
// Also wires up AndroidInput and CameraControls::set_callbacks() (issue #49) - touch-drag
// orbit and pinch both go through the same ClientHidEvent queue mouse-drag/scroll already
// do, so there's no scene/renderer here to visibly confirm against yet (same blocker
// below), but the touch-event plumbing, gesture math, and CameraControls wiring are all
// real and exercised end to end down to the event queue.
//
// Also registers an AndroidAssetProvider and attempts to load the Suzanne glTF through it
// (issue #50) - this exercises the real AssetProviderI::readBytes()/tinygltf FsCallbacks
// path, but there is currently no build step that packages any assets into an APK at all
// (that's its own gap, out of scope for every issue in this breakdown - see the PR
// description), so on an actual device this is expected to log a "not found" failure
// rather than success until that packaging exists. What's being verified here is that the
// code path runs without crashing and reports a real AAssetManager error, not that the
// model actually loads.
//
// Also exercises imgui_impl_android (issue #51) - creates a throwaway ImGui context,
// initializes/tears down the Android platform backend against the real ANativeWindow, and
// computes the device's font/widget scale from AConfiguration_getDensity(). Like
// verifyVulkanSurfaceCreation() above, there's no persistent VkRenderer/Gui here yet to
// keep a real context alive for (VkRenderer::initGui()/Gui::drawGui() are real and Android-
// ready, same as the Vulkan surface/lifecycle plumbing above), so this is a self-contained
// init/shutdown smoke test rather than a live one wired into the event loop below.
#include <android/configuration.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <stdexcept>

#include "application/Application.hpp"
#include "filesystem/android/AndroidAssetProvider.hpp"
#include "hid/android/AndroidInput.hpp"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "renderer/vk/Instance.hpp"
#include "scene/Gltf.hpp"
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

void verifyAssetLoading() {
    tinygltf::Model model;
    try {
        if (ENG::load_gltf_model(ENG::getAssetProvider().getGltfDir(), model)) {
            LOGI("AndroidAssetProvider loaded the glTF model successfully.");
        } else {
            // load_gltf_model() logs its own warn/err via ENG_LOG_* on failure - this
            // just confirms it returned rather than crashing.
            LOGE("AndroidAssetProvider: glTF load reported failure (see above).");
        }
    } catch (const std::exception& e) {
        LOGE("AndroidAssetProvider: glTF load threw: %s", e.what());
    }
}

// Android's screen density range is much wider than desktop's default ~96 DPI assumption
// that ImGui's font/widget sizing is tuned for (see issue #51) - VkRenderer::initGui()'s
// fontScale parameter expects a multiplier relative to the ACONFIGURATION_DENSITY_MEDIUM
// (160 DPI) baseline, computed here the same way a future real Android VkRenderer caller
// would.
float computeFontScale(android_app* app) {
    const int32_t density = AConfiguration_getDensity(app->config);
    if (density <= 0) {
        LOGE("AConfiguration_getDensity returned %d, defaulting fontScale to 1.0", density);
        return 1.0f;
    }
    return static_cast<float>(density) / static_cast<float>(ACONFIGURATION_DENSITY_MEDIUM);
}

void verifyImguiAndroidBackend(AndroidWindow& window, android_app* app) {
    const float fontScale = computeFontScale(app);
    LOGI("Computed ImGui fontScale = %f from device density.", fontScale);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().FontGlobalScale = fontScale;

    if (!ImGui_ImplAndroid_Init(reinterpret_cast<ANativeWindow*>(window.nativeHandle()))) {
        LOGE("ImGui_ImplAndroid_Init failed.");
        ImGui::DestroyContext();
        return;
    }
    LOGI("ImGui_ImplAndroid_Init succeeded.");

    // Deliberately doesn't call ImGui_ImplAndroid_NewFrame()/ImGui::NewFrame() here -
    // confirmed on a real device (see PR discussion) that ImGui::NewFrame() null-derefs
    // atlas->Builder unless a renderer backend (ImGui_ImplVulkan_Init(), which builds the
    // font atlas and sets ImGuiBackendFlags_RendererHasTextures) has already run first.
    // VkRenderer::initGui() does call that before Gui::drawGui() ever reaches NewFrame(),
    // so the real code path is unaffected - there's just no renderer here to stand up for
    // this isolated platform-backend smoke test (same #59 blocker as everything else in
    // this file).

    ImGui_ImplAndroid_Shutdown();
    ImGui::DestroyContext();
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

    ENG::AndroidAssetProvider assetProvider(app->activity->assetManager);
    ENG::setAssetProvider(assetProvider);
    verifyAssetLoading();

    AndroidWindow window(app);
    verifyVulkanSurfaceCreation(window);
    verifyImguiAndroidBackend(window, app);

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
