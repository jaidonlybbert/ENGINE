#ifndef ENG_MESH_DEF
#define ENG_MESH_DEF

#include<vector>
#include<string>
#include<iostream>
#include<memory>
#include<cstring>
#include<variant>
#include<filesystem>
#include<optional>

#include "vulkan/vulkan_core.h"
#include "glm/glm.hpp"
#include "renderer/vk/Buffer.hpp"
#include "renderer/vk/Command.hpp"
#include "scene/Primitives.hpp"
#include "logger/Logging.hpp"

namespace tinygltf {
class Accessor;
class Primitive;
class Model;
}

// Helper for combining multiple lambdas
template<class... Ts> struct overload : Ts... { using Ts::operator()...; };
template<class... Ts> overload(Ts...) -> overload<Ts...>;

namespace ENG
{
	using VertexT = std::variant<
		std::vector<VertexPosColTex>,
		std::vector<VertexPosNorCol>,
		std::vector<VertexPosNorTex>,
		std::vector<VertexPos>
	>;

	struct HostMeshData {
		VertexT vertexBuffer;
		std::vector<uint32_t> indexBuffer;
		std::string shaderId;
		std::optional<std::filesystem::path> texturePath;
	};

	struct BindHostMeshDataEvent {
		HostMeshData meshData;
		uint32_t nodeId;
	};

	size_t get_size_bytes_from_tinygltf_accessor(const tinygltf::Accessor& acc);

	class Mesh {

	public:
		explicit Mesh(const VkDevice& device, const VkPhysicalDevice& physicalDevice, ENG::Command* const commands,
			std::string name, VertexT vertices, std::vector<uint32_t> indices, const VkQueue& graphicsQueue);

		explicit Mesh(const VkDevice& device, const VkPhysicalDevice& physicalDevice, ENG::Command* const commands, const std::string& mesh_name, const tinygltf::Model& model,
			const tinygltf::Primitive& primitive, const VkQueue& graphicsQueue, const VertexPosColTex* meshType = nullptr);
		explicit Mesh(const VkDevice& device, const VkPhysicalDevice& physicalDevice, ENG::Command* const commands, const std::string& mesh_name, const tinygltf::Model& model,
			const tinygltf::Primitive& primitive, const VkQueue& graphicsQueue, const VertexPosNorTex* meshType = nullptr);
		explicit Mesh(const VkDevice& device, const VkPhysicalDevice& physicalDevice, ENG::Command* const commands, const std::string& mesh_name, const tinygltf::Model& model,
			const tinygltf::Primitive& primitive, const VkQueue& graphicsQueue, const VertexPos* meshType = nullptr);

		std::string name{};
		VertexT vertices{};
		std::vector<uint32_t> indices{};
		std::unique_ptr<ENG::Buffer> vertexBuffer;
		std::unique_ptr<ENG::Buffer> indexBuffer;
		const VkDevice& device;
		const VkPhysicalDevice& physicalDevice;
		ENG::Command* const commands;
		const VkQueue& graphicsQueue;

		template<typename T>
		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(T);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		template<typename T>
		static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

		void createVertexBuffer(const VkQueue& graphicsQueue)
		{
			ENG_LOG_TRACE("Entering create vertex buffer" << std::endl);
			size_t vert_size_in_bytes{ 0 };
			auto number_of_vertices{ 0 };
			void* data_ptr{ nullptr };

			std::visit(overload{
				[&vert_size_in_bytes, &number_of_vertices, &data_ptr](const std::vector<VertexPosColTex>& v) {vert_size_in_bytes = sizeof(VertexPosColTex); number_of_vertices = v.size(); data_ptr = (void*)(v.data()); },
				[&vert_size_in_bytes, &number_of_vertices, &data_ptr](const std::vector<VertexPosNorCol>& v) {vert_size_in_bytes = sizeof(VertexPosNorCol); number_of_vertices = v.size(); data_ptr = (void*)(v.data()); },
				[&vert_size_in_bytes, &number_of_vertices, &data_ptr](const std::vector<VertexPosNorTex>& v) {vert_size_in_bytes = sizeof(VertexPosNorTex); number_of_vertices = v.size(); data_ptr = (void*)(v.data()); },
				[&vert_size_in_bytes, &number_of_vertices, &data_ptr](const std::vector<VertexPos>& v) {vert_size_in_bytes = sizeof(VertexPos); number_of_vertices = v.size(); data_ptr = (void*)(v.data()); },
				}, vertices);

			const VkDeviceSize bufferSize = vert_size_in_bytes * number_of_vertices;

			const ENG::Buffer stagingBuffer{ device, physicalDevice, vert_size_in_bytes, bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

			void* data;
			vkMapMemory(device, stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
			char* last_byte = (char*)data + bufferSize;
			assert((char*)data + bufferSize <= last_byte);
			memcpy(data, data_ptr, bufferSize);
			data = (char*)data + bufferSize;
			vkUnmapMemory(device, stagingBuffer.bufferMemory);

			vertexBuffer = std::make_unique<ENG::Buffer>(device, physicalDevice, vert_size_in_bytes, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (commands == nullptr)
			{
				ENG_LOG_ERROR("mesh.hpp - commands is nullptr!" << std::endl);
				return;
			}
			commands->copyBuffer(graphicsQueue, stagingBuffer.buffer, vertexBuffer->buffer, bufferSize);
			ENG_LOG_TRACE("Vertex buffer created successfully" << std::endl);
		}

		void createIndexBuffer(const VkQueue& graphicsQueue)
		{
			size_t idx_size = sizeof(indices[0]);
			VkDeviceSize bufferSize = idx_size * indices.size();
			ENG_LOG_TRACE("buffer size: " << bufferSize << std::endl);

			const ENG::Buffer stagingBuffer{ device, physicalDevice, idx_size, bufferSize,
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT };

			void* data;
			vkMapMemory(device, stagingBuffer.bufferMemory, 0, bufferSize, 0, &data);
			char* last_byte = (char*)data + bufferSize;
			size_t buffSize = (size_t)idx_size * indices.size();
			assert((char*)data + buffSize <= last_byte);
			memcpy(data, indices.data(), buffSize);
			data = (char*)data + buffSize;
			vkUnmapMemory(device, stagingBuffer.bufferMemory);

			indexBuffer = std::make_unique<ENG::Buffer>(device, physicalDevice, idx_size, bufferSize,
				VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			commands->copyBuffer(graphicsQueue, stagingBuffer.buffer, indexBuffer->buffer, bufferSize);

			ENG_LOG_TRACE("idx buffer created successfully" << std::endl);
		}
	};
} // end namespace


#endif
