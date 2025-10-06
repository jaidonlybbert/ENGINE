#include<iostream>
#include<array>
#include "interfaces/Logging.hpp"
#include "pipelines/Pipeline.hpp"
#include "primitives/Mesh.hpp"
#include "pipelines/ShaderFactory.hpp"
#include "pipelines/PipelinePosColTex.hpp"

namespace ENG {

Pipeline_PosColTex::Pipeline_PosColTex(const VkDevice& device, const VkRenderPass& renderPass,
	const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos) : Pipeline(device)
{
	Initialize(renderPass, shader_fac, pipelineCreateInfos);
}

void Pipeline_PosColTex::createShaderStages(const ShaderFactory& shader_fac) {
	ENG_LOG_INFO("Create shaders derived class PosColTex" << std::endl);
	shader_stages = shader_fac.get_shader_stages(ENG_SHADER::PosColTex);
	assert (shader_stages.size() == 2);
	assert (shader_stages.at(0) && shader_stages.at(1));
}

void Pipeline_PosColTex::createVertexInputInfo() {
	ENG_LOG_INFO("Create vertex input info derived class PosColTex" << std::endl);
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
	ENG_LOG_INFO("Create shaders derived class PosNorTex" << std::endl);
	shader_stages = shader_fac.get_shader_stages(ENG_SHADER::PosNorTex);
	assert (shader_stages.size() == 2);
	assert (shader_stages.at(0) && shader_stages.at(1));
}

void Pipeline_PosNorTex::createVertexInputInfo() {
	ENG_LOG_INFO("Create vertex input info derived class PosNorTex" << std::endl);
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

Pipeline_PosNorCol::Pipeline_PosNorCol(const VkDevice& device, const VkRenderPass& renderPass,
	const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos) : Pipeline(device)
{
	Initialize(renderPass, shader_fac, pipelineCreateInfos);
}

void Pipeline_PosNorCol::createShaderStages(const ShaderFactory& shader_fac) {
	ENG_LOG_INFO("Create shaders derived class PosNorCol" << std::endl);
	shader_stages = shader_fac.get_shader_stages(ENG_SHADER::PosNorCol);
	assert (shader_stages.size() == 2);
	assert (shader_stages.at(0) && shader_stages.at(1));
}

void Pipeline_PosNorCol::createVertexInputInfo() {
	ENG_LOG_INFO("Create vertex input info derived class PosNorCol" << std::endl);
	attributeDescriptions = Mesh<VertexPosNorCol>::getAttributeDescriptions();
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VertexPosNorCol);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}

Pipeline_PosBB::Pipeline_PosBB(const VkDevice& device, const VkRenderPass& renderPass,
	const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos) : Pipeline(device)
{
	Initialize(renderPass, shader_fac, pipelineCreateInfos);
}

void Pipeline_PosBB::createShaderStages(const ShaderFactory& shader_fac) {
	ENG_LOG_INFO("Create shaders derived class PosBB" << std::endl);
	shader_stages = shader_fac.get_shader_stages(ENG_SHADER::PosBB);
	assert (shader_stages.size() == 2);
	assert (shader_stages.at(0) && shader_stages.at(1));
}

void Pipeline_PosBB::createVertexInputInfo() {
	ENG_LOG_INFO("Create vertex input info derived class PosBB" << std::endl);
	attributeDescriptions = Mesh<VertexPos>::getAttributeDescriptions();
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(VertexPos);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
}


void Pipeline_PosBB::createInputAssemblyInfo() {
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;
}

void Pipeline_PosBB::createRasterizationStateInfo() {
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // doesn't matter for lines
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_NONE;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f;
	rasterizer.depthBiasClamp = 0.0f;
	rasterizer.depthBiasSlopeFactor = 0.0f;
}


void Pipeline_PosNorCol::createDescriptorSetLayout(const VkDevice& device) {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding modelMatrixBinding{};
	modelMatrixBinding.binding = 1;
	modelMatrixBinding.descriptorCount = 1;
	modelMatrixBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	modelMatrixBinding.pImmutableSamplers = nullptr;
	modelMatrixBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, modelMatrixBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

void Pipeline_PosBB::createDescriptorSetLayout(const VkDevice& device) {
	VkDescriptorSetLayoutBinding uboLayoutBinding{};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	VkDescriptorSetLayoutBinding modelMatrixBinding{};
	modelMatrixBinding.binding = 1;
	modelMatrixBinding.descriptorCount = 1;
	modelMatrixBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	modelMatrixBinding.pImmutableSamplers = nullptr;
	modelMatrixBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,  modelMatrixBinding};
	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
} // end namespace
