#include<vector>
#include "tiny_obj_loader.h"
#include "scene/Mesh.hpp"
#include "filesystem/FilesystemInterface.hpp"
#include "scene/Scene.hpp"
#include "scene/Obj.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"

namespace ENG
{

void loadModel(
	VkAdapter& adapter,
	std::string name, 
	const std::filesystem::path& objPath, 
	const std::filesystem::path& texturePath,
	SceneState &sceneState, 
	Node& attachmentPoint) 
{
	auto& newNode = sceneState.graph.create_node();
	newNode.name = name;
	newNode.parent = &attachmentPoint;
	attachmentPoint.children.push_back(&newNode);
		
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	std::vector<VertexPosColTex> vertices;
	std::vector<uint32_t> indices;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.string().c_str(), get_mtl_dir().string().c_str())) {
		throw std::runtime_error(warn + err);
	}

	auto texPath = texturePath;

	ENG_LOG_INFO("Found " << materials.size() << " materials." << std::endl);
	for (const auto& mat : materials)
	{
		ENG_LOG_INFO("\tName: " << mat.name << std::endl);
		ENG_LOG_INFO("\tDiffuse texture: " << mat.diffuse_texname << std::endl);
		texPath = (get_mtl_dir() / std::filesystem::path(mat.diffuse_texname)).lexically_normal();
		ENG_LOG_INFO("\tConcat path: " << texPath << std::endl);
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

	adapter.graphicsEventQueue.push(
		BindHostMeshDataEvent{
			HostMeshData {
				std::move(vertices),
				std::move(indices),
				"VertexPosColTex",
				"PosColTex",
				texPath
			},
			newNode.nodeId
		}
	);
}

} // end namespace
