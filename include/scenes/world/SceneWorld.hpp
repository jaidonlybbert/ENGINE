#pragma once
#include "renderer/RenderAdapterI.hpp"
#include "scene/Scene.hpp"

void create_world_polyhedra(ENG::SceneState& sceneState);
// void addBoundingBoxChild(ENG::Node* node, const std::string &bbName, SceneState& sceneState);
void initializeWorldScene(ENG::SceneState& sceneState, RenderAdapterI& renderAdapter);
