#include<vector>
#include "tiny_obj_loader.h"
#include "primitives/Mesh.hpp"
#include "interfaces/FilesystemInterface.hpp"
#include "interfaces/Scene.hpp"
#include "interfaces/Obj.hpp"

namespace ENG
{

void loadModel(const VkDevice& device, const VkPhysicalDevice &physicalDevice, ENG::Command* const commands,
      std::string name, const VkQueue &graphicsQueue,
      const std::filesystem::path &filepath, SceneState &sceneState, Node& attachmentPoint) {

	auto& newNode = sceneState.graph.nodes.emplace_back();
	newNode.name = name;
	newNode.shaderId = ENG_SHADER::PosColTex;
	newNode.parent = &attachmentPoint;
	attachmentPoint.children.push_back(&newNode);
		
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	std::vector<VertexPosColTex> vertices;
	std::vector<uint32_t> indices;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.string().c_str())) {
		throw std::runtime_error(warn + err);
	}

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			VertexPosColTex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = {1.0f, 1.0f, 1.0f};

			vertices.push_back(vertex);
			indices.push_back(indices.size());
		}
	}

	auto& mesh = sceneState.posColTexMeshes.emplace_back(device, physicalDevice, commands, name, vertices, indices, graphicsQueue);
	auto* meshPtr = dynamic_cast<Component*>(&mesh);
	newNode.mesh = meshPtr;
}

} // end namespace
