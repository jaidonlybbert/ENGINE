#ifndef ENG_MESH_DEF
#define ENG_MESH_DEF

#include<vector>
#include "vulkan/vulkan_core.h"
#include "glm/glm.hpp"

namespace tinygltf {
class Accessor;
class Primitive;
class Model;
}

struct VertexPosNorTex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct VertexPosColTex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
};

class Component {
	/*
	 * Base class for components associated with Entities
	 */
};

static size_t get_size_bytes_from_tinygltf_accessor(const tinygltf::Accessor& acc);

template <typename T>
class Mesh : Component {

public:
	Mesh() {}
	Mesh(std::string name, std::vector<T> vertices, std::vector<uint32_t> indices) : name(name), vertices(vertices), indices(indices) {}
	Mesh(const std::string& mesh_name, const tinygltf::Model& model, const tinygltf::Primitive& primitive);

	std::string name{};
	std::vector<T> vertices{};
	std::vector<uint32_t> indices{};

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(T);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

// template class Mesh<VertexPosColTex>;
// template class Mesh<VertexPosNorTex>;
// template<> Mesh<VertexPosColTex>::Mesh(const std::string& mesh_name, const tinygltf::Model& model, const tinygltf::Primitive& primitive);
// template<> Mesh<VertexPosNorTex>::Mesh(const std::string& mesh_name, const tinygltf::Model& model, const tinygltf::Primitive& primitive);
// template<> std::vector<VkVertexInputAttributeDescription> Mesh<VertexPosColTex>::getAttributeDescriptions();
// template<> std::vector<VkVertexInputAttributeDescription> Mesh<VertexPosNorTex>::getAttributeDescriptions();

#endif
