#pragma once
#include "scene/Scene.hpp"
#include "renderer/vk/Renderer.hpp"
#include "renderer/RendererI.hpp"

struct alignas(CACHE_LINE_SIZE) DrawData
{
	VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];
	VkBuffer vertexBuffers[1];
	VkDeviceSize vertexBufferOffsets[1];
	VkBuffer indexBuffer;
	uint32_t numberOfElements;
};


class VkAdapter : RenderAdapterI
{

	std::vector<void*> mappedDevicePtrs;
	std::vector<VkBuffer> vkBuffers;

	void* allocateDeviceMemory(const std::size_t size_bytes, const std::size_t swap_count) override
	{
		return nullptr;
	}

	void deallocateDeviceMemory(void* devMemoryPtr) override
	{

	}
};


template <typename T>
void validateMesh(const ENG::Mesh<T>& mesh)
{
	assert(mesh.vertexBuffer != nullptr);
	assert(mesh.vertexBuffer->buffer != nullptr);
	assert(mesh.indexBuffer != nullptr);
	assert(mesh.indexBuffer->buffer != nullptr);
}
	

template <typename T>
void recordDrawCommand(VkCommandBuffer& commandBuffer, const ENG::Mesh<T>& mesh)
{
	validateMesh<T>(mesh);
	VkBuffer vertexBuffers[] = {mesh.vertexBuffer->buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
}

void recordDrawMeshCommand(VkCommandBuffer& commandBuffer, const std::string& mesh_type, const size_t mesh_idx, const SceneState& sceneState);

void recordCommandsForSceneGraph(VkRenderer& renderer, VkCommandBuffer& commandBuffer, SceneState& sceneState);
