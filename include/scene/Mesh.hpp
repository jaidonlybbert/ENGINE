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

	class Mesh {

	public:

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
	};

} // end namespace

#endif
