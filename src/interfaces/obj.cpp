#include<vector>
#include "tiny_obj_loader.h"
#include "primitives/mesh.hpp"
#include "interfaces/FilesystemInterface.hpp"
#include "interfaces/scene.h"
#include "interfaces/obj.h"

namespace ENG
{

void loadModel(SceneState &sceneState) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	std::vector<VertexPosColTex> vertices;
	std::vector<uint32_t> indices;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, get_model_dir().string().c_str())) {
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

	const auto& meshName = std::string("Room");
	const auto& mesh = Mesh<VertexPosColTex>(meshName, vertices, indices);
	sceneState.posColTexMeshes.push_back(mesh);
}

} // end namespace
