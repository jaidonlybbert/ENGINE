#include "pipelines/Pipeline.hpp"
#include "primitives/mesh.hpp"
#include "pipelines/shader_factory.hpp"
#include "pipelines/Pipeline_PosColTex.hpp"
#include<iostream>

namespace ENG {

Pipeline_PosColTex::Pipeline_PosColTex(const VkDevice& device, const VkRenderPass& renderPass,
	const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos) : Pipeline(device)
{
	Initialize(renderPass, shader_fac, pipelineCreateInfos);
}

void Pipeline_PosColTex::createShaderStages(const ShaderFactory& shader_fac) {
	std::cout << "Create shaders derived class PosColTex" << std::endl;
	shader_stages = shader_fac.get_shader_stages(ENG_SHADER::PosColTex);
	assert (shader_stages.size() == 2);
	assert (shader_stages.at(0) && shader_stages.at(1));
}

void Pipeline_PosColTex::createVertexInputInfo() {
	std::cout << "Create vertex input info derived class PosColTex" << std::endl;
	attributeDescriptions = Mesh<VertexPosColTex>::getAttributeDescriptions();
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VertexPosColTex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}

Pipeline_PosNorTex::Pipeline_PosNorTex(const VkDevice& device, const VkRenderPass& renderPass,
	const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos) : Pipeline(device)
{
	Initialize(renderPass, shader_fac, pipelineCreateInfos);
}

void Pipeline_PosNorTex::createShaderStages(const ShaderFactory& shader_fac) {
	std::cout << "Create shaders derived class PosNorTex" << std::endl;
	shader_stages = shader_fac.get_shader_stages(ENG_SHADER::PosNorTex);
	assert (shader_stages.size() == 2);
	assert (shader_stages.at(0) && shader_stages.at(1));
}

void Pipeline_PosNorTex::createVertexInputInfo() {
	std::cout << "Create vertex input info derived class PosNorTex" << std::endl;
	attributeDescriptions = Mesh<VertexPosNorTex>::getAttributeDescriptions();
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VertexPosNorTex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}
} // end namespace
