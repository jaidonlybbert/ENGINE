#pragma once
#include "hid/InputI.hpp"
#include "renderer/RenderAdapterI.hpp"
#include "scene/Scene.hpp"
#include "window/WindowI.hpp"

void create_world_polyhedra(ENG::SceneState& sceneState);
// void addBoundingBoxChild(ENG::Node* node, const std::string &bbName, SceneState& sceneState);
void initializeWorldScene(ENG::SceneState& sceneState, RenderAdapterI& renderAdapter, WindowI& window, InputI& input,
                          WindowUserData& windowUserData);
