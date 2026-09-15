// Android entry point (android_native_app_glue calls this instead of main()). Constructs a
// real, persistent VkRenderer and runs SceneBlueSky (see issue #59) - the first time any of
// #47-#51's Android-specific code has actually been exercised end to end by a live render
// loop, rather than each being proven in isolation via a throwaway verify*() harness that
// immediately tore down whatever it created.
//
// SceneBlueSky, not SceneWorld, is the scene used here: it is entirely procedural (a
// skybox + a tetrahedron generated in code - see SceneBlueSky.cpp), so it never touches
// AssetProviderI's gltf/obj/texture-loading paths. Those paths are real and Android-ready
// (issue #50), but VkRenderer::initVulkan() unconditionally preloads two specific desktop
// textures (getRoomTex()/getSpacefloorTex()) regardless of which scene is active - SceneWorld
// itself would additionally exercise loadModel()/load_gltf() end to end, which has never
// been proven through the full Vulkan texture-upload path on Android. Keeping that
// combination isolated (rather than debugging two new things - a persistent renderer AND
// full asset-backed content - at once) is why SceneWorld is left for a future issue.
//
// Also still verifies the AssetProviderI::readBytes()/tinygltf FsCallbacks path directly
// (see verifyAssetLoading() below, issue #50) - SceneBlueSky never exercises it, so this is
// kept as independent confirmation that it still works, alongside (not instead of) the real
// render loop.
//
// Application's lifecycle hooks (issue #48) now do real work: registerSurfaceDestroyedCallback/
// registerSurfaceCreatedCallback call VkRenderer::handleSurfaceDestroyed()/
// handleSurfaceCreated() on the live renderer (APP_CMD_TERM_WINDOW/APP_CMD_INIT_WINDOW),
// instead of just logging. Touch input (issue #49) drives CameraControls exactly as it does
// on desktop - SceneBlueSky's own initializeBlueSkyScene() wires CameraControls::set_callbacks()
// internally, the same call desktop's initializeScene() makes.
#include <android/configuration.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <stdexcept>

#include "application/Application.hpp"
#include "filesystem/android/AndroidAssetProvider.hpp"
#include "gui/Gui.hpp"
#include "guis/SceneGui.hpp"
#include "hid/android/AndroidInput.hpp"
#include "imgui.h"
#include "imgui_impl_android.h"
#include "renderer/vk/Instance.hpp"
#include "renderer/vk_adapter/PipelineFactory.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"
#include "scene/DFT.hpp"
#include "scene/Gltf.hpp"
#include "scenes/blue_sky/SceneBlueSky.hpp"
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
// (160 DPI) baseline.
float computeFontScale(android_app* app) {
    const int32_t density = AConfiguration_getDensity(app->config);
    if (density <= 0) {
        LOGE("AConfiguration_getDensity returned %d, defaulting fontScale to 1.0", density);
        return 1.0f;
    }
    return static_cast<float>(density) / static_cast<float>(ACONFIGURATION_DENSITY_MEDIUM);
}

// The following mirror main.cpp's file-local equivalents of the same name (gameLoop's
// helpers) - they aren't exposed via any shared header (main.cpp's are static/anonymous
// too), so this duplicates the minimal subset SceneBlueSky's render loop actually needs.
// Left out entirely: Lua, sockets/networking, physics (none of desktop main.cpp's
// engine::sockets/engine::physics/lua wiring is linked into the Android build, and
// SceneBlueSky needs none of it), and the mouse-hover raycast helpers (dead code even in
// main.cpp - registerUniformBufferConsumer's call site is commented out there too).

UniformBufferObject createUniformBufferObject(const ENG::SceneState& sceneState, float aspectRatio) {
    UniformBufferObject ubo{};

    glm::vec3 cam_pos = glm::vec4(0, 0, 1, 0);
    glm::vec3 up = glm::vec4(0, 1, 0, 0);
    auto fovy = 1.0;
    auto aspectScale = 1.0;
    auto znear = 0.1;
    auto zfar = 100.0;
    if (sceneState.activeCameraNodeIdx.has_value() &&
        sceneState.activeCameraNodeIdx.value() < sceneState.graph.nodes.size()) {
        const auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx.value());
        const auto* cameraPtr = dynamic_cast<ENG::Camera*>(cameraNode.camera);
        cam_pos = glm::vec3(sceneState.modelMatrices.at(cameraNode.nodeId)[3]);
        up = sceneState.modelMatrices.at(cameraNode.nodeId) * glm::vec4(0., 1., 0., 0.);
        fovy = cameraPtr->fovy;
        aspectScale = cameraPtr->aspectRatioScale;
        znear = cameraPtr->znear;
        zfar = cameraPtr->zfar;
    }

    const auto aspect = static_cast<double>(aspectRatio) * aspectScale;

    ubo.view = glm::lookAt(cam_pos, glm::vec3(0.0f, 0.0f, 0.0f), glm::normalize(up));
    ubo.proj = glm::perspective(fovy, aspect, znear, zfar);
    ubo.proj[1][1] *= -1;
    return ubo;
}

void updateModelMatrix(glm::mat4& modelMatrix, const ENG::Node& node) {
    modelMatrix = glm::translate(glm::mat4(1.f), node.translation) * glm::mat4_cast(node.rotation) *
                  glm::scale(glm::mat4(1.f), node.scale);
}

void updateModelMatrices(ENG::SceneState& sceneState) {
    for (const auto& node : sceneState.graph.nodes) {
        updateModelMatrix(sceneState.modelMatrices.at(node.nodeId), node);
    }

    for (auto* node : DFTraversal(sceneState.graph.root)) {
        if (node == sceneState.graph.root) {
            continue;
        }
        sceneState.modelMatrices.at(node->nodeId) =
            sceneState.modelMatrices.at(node->parent->nodeId) * sceneState.modelMatrices.at(node->nodeId);
    }
}

void handleNodeRotationPreserveYAsUpAction(const ClientHidEvent& hidEvent, ENG::SceneState& sceneState) {
    if (sceneState.activeNodeIdx >= sceneState.graph.nodes.size()) {
        LOGE("Active node idx is invalid!");
        return;
    }
    auto& activeNode = sceneState.graph.nodes.at(sceneState.activeNodeIdx);
    node_rotation_follows_input_preserve_y_as_up(activeNode, hidEvent.look_dx, hidEvent.look_dy);
}

void handleHidEvent(const ClientHidEvent& hidEvent, ENG::SceneState& sceneState) {
    for (const auto& action : hidEvent.actions) {
        if (action == Action::NODE_ROTATION_PRESERVE_Y_AS_UP) {
            handleNodeRotationPreserveYAsUpAction(hidEvent, sceneState);
        }
    }
}

void handleHIDEvents(std::deque<ClientHidEvent>& eventQueue, ENG::SceneState& sceneState) {
    while (!eventQueue.empty()) {
        const auto& clientEvent = eventQueue.front();
        handleHidEvent(clientEvent, sceneState);
        eventQueue.pop_front();
    }
}

void mesh_bind_event_handler(VkRenderer& renderer, ENG::SceneState& sceneState, VkAdapter& adapter,
                             ENG::BindHostMeshDataEvent&& bindEvent) {
    auto& hostMesh = bindEvent.meshData;
    auto& node = ENG::get_node_by_id(sceneState.graph, bindEvent.nodeId);

    if (hostMesh.texturePath.has_value()) {
        if (!renderer.textureImages.contains(hostMesh.texturePath.value())) {
            renderer.createTexture(hostMesh.texturePath.value());
        }
    }

    const auto drawIdx = adapter.emplaceDrawData({
        DrawDataProperties::CLEAR,
        {bindEvent.nodeId},
        hostMesh.texturePath,
        {std::nullopt},
        adapter.create_draw_data(std::move(hostMesh.vertexBuffer), std::move(hostMesh.indexBuffer)),
    });

    node.shaderId = bindEvent.meshData.shaderId;
    node.draw_data_idx = drawIdx;

    adapter.graphicsEventQueue.push(CommandCompletionEvent{[&adapter, &node, drawIdx] {
        adapter.set_property(drawIdx, DrawDataProperties::INDEX_BUFFERS_INITIALIZED);
        adapter.set_property(drawIdx, DrawDataProperties::VERTEX_BUFFERS_INITIALIZED);
        adapter.createDescriptorSets(drawIdx, node);
        adapter.set_property(drawIdx, DrawDataProperties::DESCRIPTOR_SETS_INITIALIZED);
    }});
}

void handleGraphicsEvents(VkRenderer& renderer, VkAdapter& adapter, ENG::SceneState& sceneState) {
    while (!adapter.graphicsEventQueue.empty()) {
        GraphicsEvent graphicsEvent{adapter.graphicsEventQueue.pop()};

        if (std::holds_alternative<ENG::BindHostMeshDataEvent>(graphicsEvent)) {
            mesh_bind_event_handler(renderer, sceneState, adapter,
                                    std::move(std::get<ENG::BindHostMeshDataEvent>(graphicsEvent)));
        } else if (std::holds_alternative<CommandRecorderEvent>(graphicsEvent)) {
            adapter.command_recorder_event_handler(std::move(std::get<CommandRecorderEvent>(graphicsEvent)));
        } else if (std::holds_alternative<CommandCompletionEvent>(graphicsEvent)) {
            std::get<CommandCompletionEvent>(graphicsEvent).commandCompletionHandler();
        }
    }
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
    // Both get every event, same as desktop's lack of ImGui-capture gating (no
    // io.WantCaptureMouse check exists there either - see Gui.cpp/main.cpp) - not adding
    // input-arbitration behavior that doesn't already exist on desktop.
    ImGui_ImplAndroid_HandleInputEvent(event);
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
    const float fontScale = computeFontScale(app);

    bool framebufferResized = false;
    PipelineFactory pipelineFactory;
    VkRenderer renderer{
        framebufferResized,
        window,
        {[&renderer]() { renderer.initVulkan(); }, [&renderer, fontScale]() { renderer.initGui(fontScale); }},
        {[&renderer]() { renderer.cleanupGui(); }, [&renderer]() { renderer.cleanupVulkan(); }},
        pipelineFactory};
    LOGI("VkRenderer initialized successfully.");

    Gui gui;
    ENG::SceneState sceneState;
    SceneGui sceneGui;
    gui.registerDrawCall([&sceneGui, &sceneState]() { sceneGui.drawGui(sceneState); });

    VkAdapter adapter{renderer};

    Application application;
    application.registerSurfaceDestroyedCallback([&renderer]() {
        renderer.handleSurfaceDestroyed();
        LOGI("Surface destroyed");
    });
    application.registerSurfaceCreatedCallback([&renderer]() {
        renderer.handleSurfaceCreated();
        LOGI("Surface (re)created");
    });
    application.registerPauseCallback([]() { LOGI("App paused"); });
    application.registerResumeCallback([]() { LOGI("App resumed"); });

    AndroidInput input;
    WindowUserData windowUserData;
    window.getWindowSize(windowUserData.windowWidthScreenCoords, windowUserData.windowHeightScreenCoords);

    // initializeBlueSkyScene() wires CameraControls::set_callbacks(window, input,
    // windowUserData) internally - same call desktop's initializeScene() makes.
    initializeBlueSkyScene(sceneState, adapter, window, input, windowUserData);
    renderer.sceneReadyToRender = true;
    LOGI("SceneBlueSky initialized successfully.");

    renderer.registerCommandRecorder([&adapter, &renderer, &sceneState](VkCommandBuffer commandBuffer) {
        adapter.recordCommandsForSceneGraph2(renderer, commandBuffer, sceneState);
    });
    renderer.registerUniformBufferProducer([&sceneState](float aspectRatio) -> UniformBufferObject {
        return createUniformBufferObject(sceneState, aspectRatio);
    });
    renderer.registerModelMatrixBufferUpdateFunction([&sceneState]() -> std::vector<glm::mat4>& {
        updateModelMatrices(sceneState);
        return sceneState.modelMatrices;
    });

    DispatchState dispatchState{&window, &application, &input};
    app->userData = &dispatchState;
    app->onAppCmd = onAppCmd;
    app->onInputEvent = onInputEvent;

    while (!window.shouldClose()) {
        window.pollEvents();
        handleHIDEvents(windowUserData.eventQueue, sceneState);
        handleGraphicsEvents(renderer, adapter, sceneState);

        gui.drawGui();
        renderer.drawFrame();
    }

    vkDeviceWaitIdle(renderer.device);
}
