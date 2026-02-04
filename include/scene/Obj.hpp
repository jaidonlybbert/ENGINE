#ifndef ENG_OBJ
#define ENG_OBJ
#include<filesystem>
#include "vulkan/vulkan_core.h"
#include "renderer/vk_adapter/VkAdapter.hpp"
#include "renderer/vk/Command.hpp"
#include "scene/Scene.hpp"

namespace ENG
{
struct SceneState;
void loadModel(
	VkAdapter& adapter,
	std::string name, 
	const std::filesystem::path &filepath, 
	SceneState &sceneState, 
	Node& attachmentPoint);
} // end namespace
#endif
