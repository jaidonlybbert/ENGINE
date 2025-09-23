#ifndef ENG_OBJ
#define ENG_OBJ
#include<filesystem>
#include "vulkan/vulkan_core.h"
#include "interfaces/command.h"
#include "interfaces/scene.h"

namespace ENG
{
struct SceneState;
void loadModel(const VkDevice& device, const VkPhysicalDevice &physicalDevice, ENG::Command* const commands,
      std::string name, const VkQueue &graphicsQueue,
      const std::filesystem::path &filepath, SceneState &sceneState, ENG::Node& attachmentPoint);
} // end namespace
#endif
