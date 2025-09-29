#include<vector>
#include "vulkan/vulkan_core.h"
#include "tiny_gltf.h"
#include "primitives/Mesh.hpp"

namespace ENG
{
	static size_t get_size_bytes_from_tinygltf_accessor(const tinygltf::Accessor& acc)
	{
		const auto& ctype = acc.componentType;
		const auto& type = acc.type;
		assert(ctype != -1);
		assert(type != -1);
		return tinygltf::GetNumComponentsInType(type) * tinygltf::GetComponentSizeInBytes(ctype);
	}

	template<>
	Mesh<VertexPosColTex>::Mesh(
		const VkDevice& device,
		const VkPhysicalDevice& physicalDevice,
		ENG::Command* const commands,
		std::string name,
		std::vector<VertexPosColTex> vertices,
		std::vector<uint32_t> indices,
		const VkQueue& graphicsQueue
	) : device(device), physicalDevice(physicalDevice), commands(commands), name(name), vertices(vertices), indices(indices), graphicsQueue(graphicsQueue)
	{
		createVertexBuffer(graphicsQueue);
		createIndexBuffer(graphicsQueue);
	}

	template<>
	Mesh<VertexPos>::Mesh(
		const VkDevice& device,
		const VkPhysicalDevice& physicalDevice,
		ENG::Command* const commands,
		std::string name,
		std::vector<VertexPos> vertices,
		std::vector<uint32_t> indices,
		const VkQueue& graphicsQueue
	) : device(device), physicalDevice(physicalDevice), commands(commands), name(name), vertices(vertices), indices(indices), graphicsQueue(graphicsQueue)
	{
		createVertexBuffer(graphicsQueue);
		createIndexBuffer(graphicsQueue);
	}

	template<>
	Mesh<VertexPosColTex>::Mesh(const VkDevice& device,
		const VkPhysicalDevice& physicalDevice,
		ENG::Command* const commands,
		const std::string& mesh_name,
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const VkQueue& graphicsQueue) : device(device), physicalDevice(physicalDevice), commands(commands), graphicsQueue(graphicsQueue) {
		std::cout << "debug PosColTex mesh entry" << std::endl;
		const auto& pos_acc = model.accessors[primitive.attributes.at("POSITION")];
		const auto& col_acc = model.accessors[primitive.attributes.at("COLOR0")];
		const auto& tex_acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
		assert(primitive.indices >= 0);
		const auto& ind_acc = model.accessors[primitive.indices];

		const auto& pos_bv = model.bufferViews[pos_acc.bufferView];
		const auto& col_bv = model.bufferViews[col_acc.bufferView];
		const auto& tex_bv = model.bufferViews[tex_acc.bufferView];
		const auto& ind_bv = model.bufferViews[ind_acc.bufferView];

		const auto& pos_buff = model.buffers[pos_bv.buffer];
		const auto& col_buff = model.buffers[col_bv.buffer];
		const auto& tex_buff = model.buffers[tex_bv.buffer];
		const auto& ind_buff = model.buffers[ind_bv.buffer];

		size_t pos_size{ get_size_bytes_from_tinygltf_accessor(pos_acc) };
		size_t col_size{ get_size_bytes_from_tinygltf_accessor(col_acc) };
		size_t tex_size{ get_size_bytes_from_tinygltf_accessor(tex_acc) };
		size_t ind_size{ get_size_bytes_from_tinygltf_accessor(ind_acc) };
		assert(pos_size == 12);
		assert(col_size == 12);
		assert(tex_size == 8);
		assert(ind_size == 2);

		const size_t num_elements = pos_bv.byteLength / pos_size;
		const size_t num_indices = ind_bv.byteLength / ind_size;
		assert(num_elements == col_bv.byteLength / col_size);
		assert(num_elements == tex_bv.byteLength / tex_size);
		assert(num_elements == ind_bv.byteLength / ind_size);

		std::cout << "Debug posCoTex pos1 " << std::endl;
		name = mesh_name;
		vertices.resize(num_elements);
		indices.resize(num_indices);
		for (size_t i = 0; i < num_elements; ++i)
		{
			VertexPosColTex vert;
			// Assumes data is not interleaved
			assert(pos_bv.byteStride == 0);
			assert(col_bv.byteStride == 0);
			assert(tex_bv.byteStride == 0);
			vert.pos = static_cast<glm::vec3>(pos_buff.data[pos_bv.byteOffset + i * pos_size]);
			vert.color = static_cast<glm::vec3>(col_buff.data[col_bv.byteOffset + i * col_size]);
			vert.texCoord = static_cast<glm::vec2>(tex_buff.data[tex_bv.byteOffset + i * tex_size]);

			vertices[i] = vert;
		}
		for (size_t i = 0; i < num_indices; ++i)
		{
			indices[i] = static_cast<uint32_t>(ind_buff.data[ind_bv.byteOffset + i * ind_size]);
		}

		std::cout << "Debug posCoTex pos2" << std::endl;

		createVertexBuffer(graphicsQueue);
		createIndexBuffer(graphicsQueue);

	}

	template<>
	Mesh<VertexPosNorTex>::Mesh(const VkDevice& device,
		const VkPhysicalDevice& physicalDevice,
		ENG::Command* const commands,
		const std::string& mesh_name,
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const VkQueue& graphicsQueue) : device(device), physicalDevice(physicalDevice), commands(commands), graphicsQueue(graphicsQueue) {
		const auto& pos_acc = model.accessors[primitive.attributes.at("POSITION")];
		const auto& nor_acc = model.accessors[primitive.attributes.at("NORMAL")];
		const auto& tex_acc = model.accessors[primitive.attributes.at("TEXCOORD_0")];
		assert(primitive.indices >= 0);
		const auto& ind_acc = model.accessors[primitive.indices];

		const auto& pos_bv = model.bufferViews[pos_acc.bufferView];
		const auto& nor_bv = model.bufferViews[nor_acc.bufferView];
		const auto& tex_bv = model.bufferViews[tex_acc.bufferView];
		const auto& ind_bv = model.bufferViews[ind_acc.bufferView];

		const auto& pos_buff = model.buffers[pos_bv.buffer];
		const auto& nor_buff = model.buffers[nor_bv.buffer];
		const auto& tex_buff = model.buffers[tex_bv.buffer];
		const auto& ind_buff = model.buffers[ind_bv.buffer];

		size_t pos_size{ get_size_bytes_from_tinygltf_accessor(pos_acc) };
		size_t nor_size{ get_size_bytes_from_tinygltf_accessor(nor_acc) };
		size_t tex_size{ get_size_bytes_from_tinygltf_accessor(tex_acc) };
		size_t ind_size{ get_size_bytes_from_tinygltf_accessor(ind_acc) };
		assert(pos_size == 12);
		assert(nor_size == 12);
		assert(tex_size == 8);
		assert(ind_size == 2);

		const size_t num_elements = pos_bv.byteLength / pos_size;
		const size_t num_indices = ind_bv.byteLength / ind_size;
		assert(num_elements == nor_bv.byteLength / nor_size);
		assert(num_elements == tex_bv.byteLength / tex_size);

		name = mesh_name;
		vertices.resize(num_elements);
		indices.resize(num_indices);
		for (size_t i = 0; i < num_elements; ++i)
		{
			VertexPosNorTex vert;
			// Assumes data is not interleaved
			assert(pos_bv.byteStride == 0);
			assert(nor_bv.byteStride == 0);
			assert(tex_bv.byteStride == 0);
			vert.pos = static_cast<glm::vec3>(pos_buff.data[pos_bv.byteOffset + i * pos_size]);
			vert.normal = static_cast<glm::vec3>(nor_buff.data[nor_bv.byteOffset + i * nor_size]);
			vert.texCoord = static_cast<glm::vec2>(tex_buff.data[tex_bv.byteOffset + i * tex_size]);

			vertices[i] = vert;
		}
		for (size_t i = 0; i < num_indices; ++i)
		{
			indices[i] = static_cast<uint32_t>(ind_buff.data[ind_bv.byteOffset + i * ind_size]);
		}

		createVertexBuffer(graphicsQueue);
		createIndexBuffer(graphicsQueue);
	}

	template<>
	Mesh<VertexPos>::Mesh(const VkDevice& device,
		const VkPhysicalDevice& physicalDevice,
		ENG::Command* const commands,
		const std::string& mesh_name,
		const tinygltf::Model& model,
		const tinygltf::Primitive& primitive,
		const VkQueue& graphicsQueue) : device(device), physicalDevice(physicalDevice), commands(commands), graphicsQueue(graphicsQueue) {
		const auto& pos_acc = model.accessors[primitive.attributes.at("POSITION")];
		assert(primitive.indices >= 0);
		const auto& ind_acc = model.accessors[primitive.indices];

		const auto& pos_bv = model.bufferViews[pos_acc.bufferView];
		const auto& ind_bv = model.bufferViews[ind_acc.bufferView];

		const auto& pos_buff = model.buffers[pos_bv.buffer];
		const auto& ind_buff = model.buffers[ind_bv.buffer];

		size_t pos_size{ get_size_bytes_from_tinygltf_accessor(pos_acc) };
		size_t ind_size{ get_size_bytes_from_tinygltf_accessor(ind_acc) };
		assert(pos_size == 12);
		assert(ind_size == 2);

		const size_t num_elements = pos_bv.byteLength / pos_size;
		const size_t num_indices = ind_bv.byteLength / ind_size;
		assert(num_elements == nor_bv.byteLength / nor_size);
		assert(num_elements == tex_bv.byteLength / tex_size);

		name = mesh_name;
		vertices.resize(num_elements);
		indices.resize(num_indices);
		for (size_t i = 0; i < num_elements; ++i)
		{
			VertexPos vert;
			// Assumes data is not interleaved
			assert(pos_bv.byteStride == 0);
			vert.pos = static_cast<glm::vec3>(pos_buff.data[pos_bv.byteOffset + i * pos_size]);

			vertices[i] = vert;
		}
		for (size_t i = 0; i < num_indices; ++i)
		{
			indices[i] = static_cast<uint32_t>(ind_buff.data[ind_bv.byteOffset + i * ind_size]);
		}

		createVertexBuffer(graphicsQueue);
		createIndexBuffer(graphicsQueue);
	}
		

	template<>
	std::vector<VkVertexInputAttributeDescription> Mesh<VertexPosColTex>::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 3 };

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexPosColTex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(VertexPosColTex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(VertexPosColTex, texCoord);

		return attributeDescriptions;
	}

	template<>
	std::vector<VkVertexInputAttributeDescription> Mesh<VertexPosNorTex>::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 3 };

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexPosNorTex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(VertexPosNorTex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(VertexPosNorTex, texCoord);

		return attributeDescriptions;
	}

	template<>
	std::vector<VkVertexInputAttributeDescription> Mesh<VertexPos>::getAttributeDescriptions() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 1 };

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexPos, pos);

		return attributeDescriptions;
	}
} // end namespace
