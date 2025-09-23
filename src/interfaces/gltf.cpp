#include "vulkan/vulkan.h"
#include "interfaces/gltf.h"
#include<iostream>

namespace ENG
{

static VkFormat get_vk_format_from_tinygltf_accessor(const tinygltf::Accessor& acc, size_t& size_bytes)
{
	VkFormat retval{ VkFormat::VK_FORMAT_UNDEFINED };
	const auto& ctype = acc.componentType;
	const auto& type = acc.type;
	assert(ctype != -1);
	assert(type != -1);

	if (ctype == TINYGLTF_COMPONENT_TYPE_FLOAT)
	{
		retval = 
			type == TINYGLTF_TYPE_VEC2 ? VkFormat::VK_FORMAT_R32G32_SFLOAT :
			type == TINYGLTF_TYPE_VEC3 ? VkFormat::VK_FORMAT_R32G32B32_SFLOAT :
			type == TINYGLTF_TYPE_VEC4 ? VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT : VkFormat::VK_FORMAT_UNDEFINED;
	}

	size_bytes = tinygltf::GetNumComponentsInType(type) * tinygltf::GetComponentSizeInBytes(ctype);

	assert(retval != VK_FORMAT_UNDEFINED);
	return retval;
}

bool load_gltf_model(const std::filesystem::path gltf_path, tinygltf::Model& model) {
	auto loader = tinygltf::TinyGLTF();
	std::string err;
	std::string warn;

	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, gltf_path.string());

	if (!warn.empty()) {
		printf("Warn: %s\n", warn.c_str());
	}

	if (!err.empty()) {
		printf("Err: %s\n", err.c_str());
	}

	if (!ret) {
		printf("Failed to parse glTF\n");
	} else {
		printf("Load glTF successful\n");
	}

	return ret;
}

void load_gltf_mesh_attributes(const VkDevice& device,
	const VkPhysicalDevice& physicalDevice,
	const VkQueue& graphicsQueue,
	ENG::Command* const commands,
	const std::string& mesh_name,
	const tinygltf::Model& model,
	const tinygltf::Node& node,
	const tinygltf::Primitive& primitive,
	SceneState& sceneState,
	Node& eng_node)
{
	// Assumes vertex data is NOT interleaved in gltf buffer
	// Each attribute exists in a contiguous section of the gltf buffer, and gets it's own
	// binding & attribute description
	if (primitive.attributes.contains("POSITION") && primitive.attributes.contains("COLOR0")
		&& primitive.attributes.contains("TEXCOORD_0"))
	{
		auto &mesh = sceneState.posColTexMeshes.emplace_back(device, physicalDevice, commands, mesh_name, model, primitive, graphicsQueue);
		eng_node.mesh.emplace(dynamic_cast<ENG::Component*>(&mesh));
		eng_node.shaderId = ENG_SHADER::PosColTex;
	}
	else if (primitive.attributes.contains("POSITION") && primitive.attributes.contains("NORMAL")
		&& primitive.attributes.contains("TEXCOORD_0"))
	{
		auto& mesh = sceneState.posNorTexMeshes.emplace_back(device, physicalDevice, commands, mesh_name, model, primitive, graphicsQueue);
		eng_node.mesh.emplace(dynamic_cast<ENG::Component*>(&mesh));
		eng_node.shaderId = ENG_SHADER::PosNorTex;
	}
}

void load_gltf_node(const VkDevice& device,
	const VkPhysicalDevice& physicalDevice,
	const VkQueue& graphicsQueue,
	ENG::Command* const commands,
	const tinygltf::Node& node,
	SceneState& sceneState,
	ENG::Node& eng_node)
{
	const auto& model = sceneState.scene;

	if (node.mesh < 0) return;

	const auto& gltf_mesh = model.meshes[node.mesh];
	for (const auto& primitive : gltf_mesh.primitives)
	{
		const auto& mesh_name = gltf_mesh.name;
		load_gltf_mesh_attributes(device, physicalDevice, graphicsQueue, commands, mesh_name, model, node, primitive, sceneState, eng_node);
	}
}

bool load_gltf(const VkDevice& device,
	const VkPhysicalDevice& physicalDevice,
	const VkQueue& graphicsQueue,
	ENG::Command* const commands,
	const std::filesystem::path gltf_path,
	SceneState& sceneState,
	Node& attachmentPoint)
{
	auto& model = sceneState.scene;
	if (!load_gltf_model(gltf_path, model)) {
		return false;
	}

	size_t idx{0};
	std::cout << "Nodes found:" << std::endl;
	for (const auto& node : model.nodes) {
		auto& newNode = sceneState.graph.nodes.emplace_back();
		attachmentPoint.children.push_back(&newNode);
		newNode.name = node.name;
		std::cout << "\t" << node.name << std::endl;
		if (node.name == "main_camera") {
			sceneState.activeCameraNodeIdx = idx;
			std::cout << "Camera node set" << std::endl;
		}
		load_gltf_node(device, physicalDevice, graphicsQueue, commands, node, sceneState, newNode);
		++idx;
	}

	return true;
}

} // end namespace
