#ifndef ENG_OBJ
#define ENG_OBJ
#include<filesystem>
#include "vulkan/vulkan_core.h"
#include "renderer/vk/Command.hpp"
#include "scene/Scene.hpp"

namespace ENG
{
struct SceneState;
void loadModel(const VkDevice& device, const VkPhysicalDevice &physicalDevice, ENG::Command* const commands,
      std::string name, const VkQueue &graphicsQueue,
      const std::filesystem::path &filepath, SceneState &sceneState, ENG::Node& attachmentPoint);
} // end namespace
#endif
