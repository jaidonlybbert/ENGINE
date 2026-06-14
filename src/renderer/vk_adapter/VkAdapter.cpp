#include "scene/Scene.hpp"
#include "renderer/vk/Renderer.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"

std::vector<DrawData> nodeIdToDrawDataMap;

void initForVulkan(
	const SceneState& sceneState
)
{
	// For node in scene:
		// create vertex and index VkBuffers
		// create VkDescriptorSet

}

void recordDrawDataCommand(
	VkCommandBuffer& commandBuffer,
	DrawData drawData,
	const bool indexedDraw
)
{
	if (!drawData.bufferAllocationInfo.has_value()) {
		ENG_LOG_ERROR("Attempted to record draw data command for drawdata without buffer allocation info" << std::endl);
	}
	auto& allocationInfo = drawData.bufferAllocationInfo.value();
    
	if (indexedDraw)
	{
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, allocationInfo.vertexBuffers, allocationInfo.vertexBufferOffsets);
		vkCmdBindIndexBuffer(commandBuffer, allocationInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDrawIndexed(commandBuffer, allocationInfo.indexCount, 1, 0, 0, 0);
	}
	else {
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, allocationInfo.vertexBuffers, allocationInfo.vertexBufferOffsets);
		vkCmdBindIndexBuffer(commandBuffer, allocationInfo.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdDraw(commandBuffer, allocationInfo.vertexCount, 1, 0, 0);
	}
}

struct PushConstants {
    uint32_t nodeId;
};

void VkAdapter::recordCommandsForSceneGraph2(VkRenderer& renderer, VkCommandBuffer& commandBuffer, SceneState& sceneState)
{
	for (const auto& node : sceneState.graph.nodes)
	{
		if (!node.visible)
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to visibility set to false" << std::endl);
			continue;
		}

		if (!node.draw_data_idx.has_value())
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to no DrawData" << std::endl);
			continue;
		}

		const auto drawDataIdx{ node.draw_data_idx.value() };

		if (!has_property(drawDataIdx, DrawDataProperties::VERTEX_BUFFERS_INITIALIZED) || 
			!has_property(drawDataIdx, DrawDataProperties::INDEX_BUFFERS_INITIALIZED))
		{
			ENG_LOG_DEBUG("Skipping draw for " << node.name << " which has unbound draw data" << std::endl);
			continue;
		}

		if (!has_property(drawDataIdx, DrawDataProperties::DESCRIPTOR_SETS_INITIALIZED))
		{
			ENG_LOG_DEBUG("Skipping draw call for " << node.name << " uninitialized descriptor sets" << std::endl);
			continue;
		}

		const auto drawDataCpy = getDrawDataFromIdx(drawDataIdx);

		if (!drawDataCpy.descriptorSets.has_value())
		{
			ENG_LOG_DEBUG("Skipping draw call for " << node.name << " without descriptor sets" << std::endl);
			continue;
		}

		const auto& descriptorSets = drawDataCpy.descriptorSets.value();
		if (descriptorSets.size() != MAX_FRAMES_IN_FLIGHT)
		{
			ENG_LOG_DEBUG("Skipping draw call for " << node.name << " missing descriptor sets" << std::endl);
			continue;
		}

		if (!node.shaderId.has_value())
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to no shaderId" << std::endl);
			continue;
		}
		const auto& shaderId = node.shaderId.value();

		ENG_LOG_TRACE("Drawing " << node.name << std::endl);
		vkCmdBindPipeline(
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				renderer.pipelineFactory->getVkPipeline(shaderId));

		vkCmdBindDescriptorSets(
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_GRAPHICS, 
				renderer.pipelineFactory->getVkPipelineLayout(shaderId),
				0, 
				1, 
				descriptorSets.data(),
				0, 
				nullptr);

        const PushConstants pushConstants {node.nodeId};
		vkCmdPushConstants(
				commandBuffer, 
				renderer.pipelineFactory->getVkPipelineLayout(shaderId), 
				VK_SHADER_STAGE_VERTEX_BIT, 
				0,
				sizeof(pushConstants),
				&pushConstants);

		const auto indexedDraw = shaderId != "PosNorCol" && shaderId != "Goldberg";
		recordDrawDataCommand(commandBuffer, drawDataCpy, indexedDraw);
	}
}

void recordDrawCommand(VkCommandBuffer& commandBuffer, const ENG::Mesh& mesh)
{
	assert(mesh.vertexBuffer != nullptr);
	assert(mesh.vertexBuffer->buffer != nullptr);
	assert(mesh.indexBuffer != nullptr);
	assert(mesh.indexBuffer->buffer != nullptr);
	VkBuffer vertexBuffers[] = {mesh.vertexBuffer->buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
	vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);
	vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
}
