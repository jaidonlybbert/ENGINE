// Android entry point (android_native_app_glue calls this instead of main()). Exercises
// AndroidWindow and the Android branch of InstanceFactory::getRequiredExtensions() /
// ENG::createWindowSurface() end to end (see issue #47): creates a real VkInstance and
// VkSurfaceKHR from the window and logs whether it worked.
//
// This deliberately doesn't go through VkRenderer::initVulkan() - that also picks a
// physical device, builds a swapchain, and preloads textures through AssetProviderI, none
// of which is Android-ready yet (issues #48/#50) - just the two things this issue is
// actually about. Wiring up the rest of the engine (window/input/rendering/filesystem
// lifecycle) is the follow-up issues #48-#51.
#include <android/log.h>
#include <android_native_app_glue.h>

#include <stdexcept>

#include "renderer/vk/Instance.hpp"
#include "window/android/AndroidWindow.hpp"

#define LOG_TAG "Engine"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

bool windowReady = false;

// Only used to detect the first APP_CMD_INIT_WINDOW - AndroidWindow's constructor takes
// over app->onAppCmd from that point forward (see its class comment).
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

    while (!window.shouldClose()) {
        window.pollEvents();
    }
}
