#ifndef ENG_GLTF
#define ENG_GLTF
#include<filesystem>
#include<optional>
#include "tiny_gltf.h"
#include "scene/Mesh.hpp"
#include "scene/Scene.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"

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
void load_gltf_mesh_attributes(
	VkAdapter& adapter,
	const tinygltf::Model& model,
	const tinygltf::Primitive& primitive,
	const uint32_t nodeId);

void load_gltf_node(
	VkAdapter& adapter,
	const tinygltf::Node& node,
	SceneState& sceneState,
	ENG::Node& eng_node,
	const tinygltf::Model& model);

bool load_gltf(
	VkAdapter& adapter,
	const std::filesystem::path gltf_path,
	SceneState& sceneState,
	Node& attachmentPoint);

} // end namespace
#endif
