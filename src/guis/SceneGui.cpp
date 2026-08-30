#include "guis/SceneGui.hpp"

#include <glm/glm.hpp>

#include "imgui.h"
#include "logger/Logging.hpp"
#include "scene/DFT.hpp"
#include "scene/Scene.hpp"

void SceneGui::MySaveFunction() { ENG_LOG_DEBUG("Save function call" << std::endl); }

void SceneGui::DrawNodeTree(ENG::Node* node) {
    if (ImGui::TreeNode(node->name.c_str())) {
        ImGui::Text("Properties");

        if (ImGui::Checkbox("Visible", &node->visible)) {
            ENG_LOG_DEBUG("Visible checked" << std::endl);

            // Set visibility of all children
            for (auto* child : DFTraversal(node)) {
                child->visible = node->visible;
            }
        }

        ImGui::SliderFloat4("Rotation", &(node->rotation.x), 0.f, 3.1f);
        ImGui::SliderFloat3("Location", &(node->translation.x), 0.f, 3.1f);

        for (const auto& child : node->children) {
            DrawNodeTree(child);  // Recursively draw children
        }
        ImGui::TreePop();
    }
}

void SceneGui::drawGui(ENG::SceneState& sceneState) {
    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to
    // learn more about Dear ImGui!).
    if (show_demo_window) {
        ImGui::ShowDemoWindow();
    }

    if (settings.showSettings) {
        ImGui::SetNextWindowPos(ImVec2(100, 100), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowCollapsed(true, ImGuiCond_FirstUseEver);
        ImGui::Begin("DebugTools");
        ImGui::Text("Camera settings");
        if (ImGui::Button("Save")) MySaveFunction();

        if (sceneState.activeCameraNodeIdx.has_value() &&
            sceneState.activeCameraNodeIdx.value() < sceneState.graph.nodes.size()) {
            auto& cameraNode = sceneState.graph.nodes.at(sceneState.activeCameraNodeIdx.value());
            auto* camera = dynamic_cast<ENG::Camera*>(cameraNode.camera);

            if (camera == nullptr) {
                throw(std::runtime_error("Camera is nullptr!"));
            }

            ImGui::SliderFloat("Aspect", &(camera->aspect), 0.0f, 10.0f);
            ImGui::SliderFloat("Fovy", &(camera->fovy), 0.0f, 1.0f);
            ImGui::SliderFloat("zfar", &(camera->zfar), 0.0f, 100.0f);
            ImGui::SliderFloat("znear", &(camera->znear), 0.0f, 10.0f);
            ImGui::InputFloat3("Camera position", &cameraNode.translation.x);
            ImGui::InputFloat3("Camera rotation", &cameraNode.rotation.x);
        }

        ImGui::Text("IDX: Name");
        for (auto& node : sceneState.graph.nodes) {
            ImGui::Text("%d: %s", node.nodeId, node.name.c_str());
        }
        DrawNodeTree(sceneState.graph.root);
        ImGui::InputInt("Active: ", &sceneState.activeNodeIdx);
        if (sceneState.activeNodeIdx < sceneState.graph.nodes.size()) {
            ImGui::SliderFloat4("Active Node Rotation",
                                &(sceneState.graph.nodes.at(sceneState.activeNodeIdx).rotation.x), 0.f, 3.1f);
            ImGui::SliderFloat3("Active Node Location",
                                &(sceneState.graph.nodes.at(sceneState.activeNodeIdx).translation.x), 0.f, 3.1f);
        }
        ImGui::End();
    }
}
