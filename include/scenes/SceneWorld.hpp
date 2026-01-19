#include "scenes/ProceduralGeometry.hpp"

void create_world_polyhedra(VkRenderer& app, SceneState& sceneState);
void addBoundingBoxChild(ENG::Node* node, VkRenderer& app, const std::string &bbName, SceneState& sceneState);
void create_tetrahedron_no_pmp(SceneState& sceneState, ConcurrentQueue<BindHostMeshDataEvent>& meshBindQueue);
void initializeWorldScene(VkRenderer& renderer, VkAdapter& adapter, SceneState& sceneState);
