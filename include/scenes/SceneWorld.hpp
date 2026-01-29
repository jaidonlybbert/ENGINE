#pragma once
#include "scenes/ProceduralGeometry.hpp"
#include "application/ConcurrentQueue.hpp"
#include "renderer/vk/Renderer.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"

void create_world_polyhedra(VkRenderer& app, SceneState& sceneState);
void addBoundingBoxChild(ENG::Node* node, VkRenderer& app, const std::string &bbName, SceneState& sceneState);
void create_tetrahedron_no_pmp(SceneState& sceneState, ConcurrentQueue<BindHostMeshDataEvent>& meshBindQueue);
void init_for_vulkan_tetrahedron(VkAdapter& adapter, SceneState& sceneState);
void initializeWorldScene(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState);
