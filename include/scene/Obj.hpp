#ifndef ENG_OBJ
#define ENG_OBJ
#include <filesystem>

#include "scene/Scene.hpp"

namespace ENG {
struct SceneState;
void loadModel(std::string name, const std::filesystem::path& objPath, const std::filesystem::path& texturePath,
               SceneState& sceneState, Node& attachmentPoint);
}  // namespace ENG
#endif
