#include "gui/Gui.hpp"

#include <glm/glm.hpp>

#include "imgui.h"
#include "imgui_impl_android.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "logger/Logging.hpp"

void Gui::registerDrawCall(std::function<void(void)> drawCall) { drawCalls.emplace_back(drawCall); }

void Gui::drawGui() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
#if defined(__ANDROID__)
    ImGui_ImplAndroid_NewFrame();
#else
    ImGui_ImplGlfw_NewFrame();
#endif
    ImGui::NewFrame();

    // call all registered draw calls
    for (auto& f : drawCalls) {
        f();
    }

    // Rendering
    ImGui::Render();
}
