#pragma once
#include "scenes/ProceduralGeometry.hpp"
#include "application/ConcurrentQueue.hpp"

void create_world_polyhedra(SceneState& sceneState);
//void addBoundingBoxChild(ENG::Node* node, const std::string &bbName, SceneState& sceneState);
void create_tetrahedron_no_pmp(SceneState& sceneState, const std::string& nodeName);
void initializeWorldScene(SceneState& sceneState, RenderAdapterI& renderAdapter);
