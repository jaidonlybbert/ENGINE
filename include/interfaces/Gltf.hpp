#ifndef ENG_GLTF
#define ENG_GLTF
#include<filesystem>
#include<optional>
#include "tiny_gltf.h"
#include "primitives/Mesh.hpp"
#include "interfaces/Scene.hpp"

namespace ENG
{

const std::map<std::string, int> GLTF_ATTRIBUTE_NAME_TO_VK_VERTEX_ATTR_LOCATION {
	{"POSITION", 0},
	{"COLOR_0", 1},
	{"TEXCOORD_0", 2},
	{"NORMAL", 3},
	{"JOINTS_0", 4},
	{"WEIGHTS_0", 5},
	{"TANGENT", 6},
	{"TEXCOORD_1", 7}
};

const std::map<int, VkFormat> TINYGLTF_COMPONENT_TYPE_TO_VKFORMAT = {
	{TINYGLTF_COMPONENT_TYPE_BYTE, VK_FORMAT_R8_UINT},
	{TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE, VK_FORMAT_R8_UINT},
	{TINYGLTF_COMPONENT_TYPE_SHORT, VK_FORMAT_R16_SINT},
	{TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, VK_FORMAT_R16_UINT},
	{TINYGLTF_COMPONENT_TYPE_INT, VK_FORMAT_R32_SINT},
	{TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, VK_FORMAT_R32_UINT},
	{TINYGLTF_COMPONENT_TYPE_FLOAT, VK_FORMAT_R32_SFLOAT},
	{TINYGLTF_COMPONENT_TYPE_DOUBLE, VK_FORMAT_R64_SFLOAT}
};

static VkFormat get_vk_format_from_tinygltf_accessor(const tinygltf::Accessor& acc, size_t& size_bytes);
bool load_gltf_model(const std::filesystem::path gltf_path, tinygltf::Model& model);
void load_gltf_mesh_attributes(const VkDevice& device,
	const VkPhysicalDevice& physicalDevice,
	const VkQueue& graphicsQueue,
	ENG::Command* const commands,
	const std::string& mesh_name,
	const tinygltf::Model& model,
	const tinygltf::Node& node,
	const tinygltf::Primitive& primitive,
	SceneState& sceneState,
	Node& eng_node);

void load_gltf_node(const VkDevice& device,
	const VkPhysicalDevice& physicalDevice,
	const VkQueue& graphicsQueue,
	ENG::Command* const commands,
	const tinygltf::Node& node,
	SceneState& sceneState,
	Node& eng_node);

bool load_gltf(const VkDevice& device,
	const VkPhysicalDevice& physicalDevice,
	const VkQueue& graphicsQueue,
	ENG::Command* const commands,
	const std::filesystem::path gltf_path,
	SceneState& sceneState,
	Node& attachmentPoint);

} // end namespace
#endif
