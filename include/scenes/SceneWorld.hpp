#pragma once
#include "renderer/RenderAdapterI.hpp"
#include "scene/Scene.hpp"

void create_world_polyhedra(ENG::SceneState& sceneState);
// void addBoundingBoxChild(ENG::Node* node, const std::string &bbName, SceneState& sceneState);
void create_tetrahedron_no_pmp(ENG::SceneState& sceneState, const std::string& nodeName);
void initializeWorldScene(ENG::SceneState& sceneState, RenderAdapterI& renderAdapter);
