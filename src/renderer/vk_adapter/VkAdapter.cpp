#include "scene/Scene.hpp"
#include "renderer/vk/Renderer.hpp"
#include "renderer/vk_adapter/VkAdapter.hpp"


void recordDrawMeshCommand(VkCommandBuffer& commandBuffer, const std::string& mesh_type, const size_t mesh_idx, const SceneState& sceneState)
{
		if (mesh_type == "VertexPosColTex")
		{
			ENG_LOG_TRACE("Drawing VertexPosColTex mesh");
			assert(mesh_idx < sceneState.posColTexMeshes.size());
			const auto& mesh = sceneState.posColTexMeshes.at(mesh_idx);
			recordDrawCommand(commandBuffer, mesh);
		}
		else if (mesh_type == "VertexPosNorTex")
		{	
			ENG_LOG_TRACE("Drawing VertexPosNorTex mesh");
			assert(mesh_idx < sceneState.posNorTexMeshes.size());
			auto& mesh = sceneState.posNorTexMeshes.at(mesh_idx);
			recordDrawCommand(commandBuffer, mesh);
		}
		else if (mesh_type == "VertexPos")
		{
			ENG_LOG_TRACE("Drawing VertexPos mesh");
			assert(mesh_idx < sceneState.posMeshes.size());
			auto& mesh = sceneState.posMeshes.at(mesh_idx);
			recordDrawCommand(commandBuffer, mesh);
		}
		else if (mesh_type == "VertexPosNorCol")
		{
			ENG_LOG_TRACE("Drawing VertexPosNorCol mesh");
			assert(mesh_idx < sceneState.posNorColMeshes.size());
			auto& mesh = sceneState.posNorColMeshes.at(mesh_idx);
			validateMesh(mesh);
			VkBuffer vertexBuffers[] = {mesh.vertexBuffer->buffer};
			VkDeviceSize offsets[] = {0};
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(mesh.vertices.size()), 1, 0, 0);
		}
}


void recordCommandsForSceneGraph(VkRenderer& renderer, VkCommandBuffer& commandBuffer, SceneState& sceneState)
{
	for (const auto& node : sceneState.graph.nodes)
	{
		if (!node.shaderId.has_value())
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to no shaderId" << std::endl);
			continue;
		}
		const auto& shaderId = node.shaderId.value();

		if (!node.mesh_idx.has_value() || !node.mesh_type.has_value())
		{	
			ENG_LOG_TRACE("Skipping draw for " << node.name << " due to no mesh" << std::endl);
			continue;
		}

		if (!node.visible)
		{
			ENG_LOG_TRACE("Skipping draw for " << node.name << "due to visibility set to false" << std::endl);
			continue;
		}
		ENG_LOG_TRACE("Drawing " << node.name << std::endl);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineFactory->getVkPipeline(shaderId));

		// During scene switching/loading, nodes will be in a partially loaded state not ready to be rendered
		if (renderer.currentFrame > node.descriptorSetIds.size()) {
			ENG_LOG_DEBUG("Skipping draw for " << node.name << " which has no descriptor sets" << std::endl);
			continue;
		}

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderer.pipelineFactory->getVkPipelineLayout(shaderId),
			  0, 1, &renderer.descriptorSets.get(node.descriptorSetIds.at(renderer.currentFrame)), 0, nullptr);
		vkCmdPushConstants(commandBuffer, renderer.pipelineFactory->getVkPipelineLayout(shaderId), VK_SHADER_STAGE_VERTEX_BIT, 0,
			sizeof(uint32_t), &node.nodeId);


		auto& mesh_type = node.mesh_type.value();
		auto mesh_idx = node.mesh_idx.value();
		recordDrawMeshCommand(commandBuffer, mesh_type, mesh_idx, sceneState);
	}
}

