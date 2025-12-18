#include "scenes/ProceduralGeometry.hpp"

void create_world_polyhedra(VkRenderer& app);
void addBoundingBoxChild(ENG::Node* node, VkRenderer& app, const std::string &bbName);
void create_tetrahedron_no_pmp(VkRenderer& app);
void initializeWorldScene(VkRenderer& app);
