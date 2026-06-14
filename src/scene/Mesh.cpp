#include<vector>
#include "vulkan/vulkan_core.h"
#include "scene/Mesh.hpp"
#include "logger/Logging.hpp"

namespace ENG
{
	template<>
	std::vector<VkVertexInputAttributeDescription> Mesh::getAttributeDescriptions<VertexPosColTex>() {
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
	std::vector<VkVertexInputAttributeDescription> Mesh::getAttributeDescriptions<VertexPosNorCol>() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 3 };

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexPosNorCol, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(VertexPosNorCol, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(VertexPosNorCol, color);

		return attributeDescriptions;
	}

	template<>
	std::vector<VkVertexInputAttributeDescription> Mesh::getAttributeDescriptions<VertexPosNorTex>() {
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
	std::vector<VkVertexInputAttributeDescription> Mesh::getAttributeDescriptions<VertexPos>() {
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions{ 1 };

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(VertexPos, pos);

		return attributeDescriptions;
	}
} // end namespace
