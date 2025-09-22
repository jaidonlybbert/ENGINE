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

void load_gltf_mesh_attributes(const VkDevice &device, 
			       const VkPhysicalDevice &physicalDevice,
			       const VkQueue &graphicsQueue,
			       ENG::Command* const commands,
			       const std::string& mesh_name,
			       const tinygltf::Model& model,
			       const tinygltf::Node& node,
			       const tinygltf::Primitive& primitive,
			       SceneState& sceneState)
{
	std::optional<std::size_t> bufferIdx;
	// Assumes vertex data is NOT interleaved in gltf buffer
	// Each attribute exists in a contiguous section of the gltf buffer, and gets it's own
	// binding & attribute description

	// guarantees unique binding index as long as less than 10 attributes
	size_t bindingIdx{static_cast<size_t>(node.mesh) * 10};  
	assert(primitive.attributes.size() < 10);

	if (primitive.attributes.contains("POSITION") && primitive.attributes.contains("COLOR0")
		&& primitive.attributes.contains("TEXCOORD_0"))
	{
		sceneState.posColTexMeshes.push_back(Mesh<VertexPosColTex>(device, physicalDevice, commands,  mesh_name,  model,
						      primitive, graphicsQueue));
	}
	else if (primitive.attributes.contains("POSITION") && primitive.attributes.contains("NORMAL")
		&& primitive.attributes.contains("TEXCOORD_0"))
	{
		std::cout << "posnortex debug: " << mesh_name << std::endl;
		sceneState.posNorTexMeshes.push_back(Mesh<VertexPosNorTex>(device, physicalDevice, commands,  mesh_name,  model,
						      primitive, graphicsQueue));
	}
}

void load_gltf_node(const VkDevice &device,
		    const VkPhysicalDevice &physicalDevice,
		    const VkQueue &graphicsQueue,
		    ENG::Command* const commands, 
		    const tinygltf::Node& node, 
		    SceneState& sceneState)
{
	const auto& model = sceneState.scene;

	if (node.mesh < 0) return;

	const auto& gltf_mesh = model.meshes[node.mesh];
	for (const auto& primitive : gltf_mesh.primitives)
	{
		const auto& mesh_name = gltf_mesh.name;
		load_gltf_mesh_attributes(device, physicalDevice, graphicsQueue, commands, mesh_name, model, node, primitive, sceneState);
	}
}

bool load_gltf(const VkDevice &device,
	       const VkPhysicalDevice &physicalDevice,
	       const VkQueue &graphicsQueue, 
	       ENG::Command* const commands, 
	       const std::filesystem::path gltf_path, 
	       SceneState& sceneState) 
{
	auto& model = sceneState.scene;
	if (!load_gltf_model(gltf_path, model)) {
		return false;
	}

	size_t idx{0};
	std::cout << "Nodes found:" << std::endl;
	for (const auto& node : model.nodes) {
		std::cout << "\t" << node.name << std::endl;
		if (node.name == "main_camera") {
			sceneState.activeCameraNodeIdx = idx;
			std::cout << "Camera node set" << std::endl;
		}
		load_gltf_node(device, physicalDevice, graphicsQueue, commands, node, sceneState);
		++idx;
	}

	return true;
}

} // end namespace
