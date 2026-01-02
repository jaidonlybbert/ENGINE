#include "scenes/ProceduralGeometry.hpp"

void create_world_polyhedra(VkRenderer& app, SceneState& sceneState);
void addBoundingBoxChild(ENG::Node* node, VkRenderer& app, const std::string &bbName, SceneState& sceneState);
void create_tetrahedron_no_pmp(VkRenderer& app, SceneState& sceneState);
void initializeWorldScene(VkRenderer& app, SceneState& sceneState);
