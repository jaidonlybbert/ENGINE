#include<vector>
#include<array>
#include<assert.h>
#include "vulkan/vulkan_core.h"
#include "renderer/pipelines/PipelineFactory.hpp"
#include "renderer/pipelines/ShaderFactory.hpp"
#include "renderer/pipelines/Pipeline.hpp"
#include "renderer/pipelines/PipelinePosColTex.hpp"

namespace ENG {
PipelineFactory::PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat,
								 const VkFormat& depthFormat) : device(device) {
	std::vector<VkGraphicsPipelineCreateInfo> pipelineCreateInfos;
	const ShaderFactory& shader_factory{device};
	createRenderPass(device, swapChainImageFormat, depthFormat);

	// TODO: interim solution to decouple renderer from scene, should be loaded from a config file
	pipeline_names.emplace("PosColTex", 0);
	pipeline_names.emplace("PosNorTex", 1);
	pipeline_names.emplace("PosBB", 2);
	pipeline_names.emplace("PosNorCol", 3);
	pipeline_names.emplace("Goldberg", 4);

	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosColTex>(device, renderPass, shader_factory, pipelineCreateInfos));
	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosNorTex>(device, renderPass, shader_factory, pipelineCreateInfos));
	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosBB>(device, renderPass, shader_factory, pipelineCreateInfos));
	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosNorCol>(device, renderPass, shader_factory, pipelineCreateInfos));
	eng_pipelines.emplace_back(std::make_unique<Pipeline_Goldberg>(device, renderPass, shader_factory, pipelineCreateInfos));

	assert(pipelineCreateInfos.size() == 5);
	graphicsPipelines.resize(pipelineCreateInfos.size());
	assert(graphicsPipelines.data() && pipelineCreateInfos.data());
	const auto* pCreateInfos = pipelineCreateInfos.data();
	auto* pPipelines = graphicsPipelines.data();
	const auto success = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, pipelineCreateInfos.size(), pCreateInfos, nullptr, pPipelines);
	if (success != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
}

PipelineFactory::~PipelineFactory()
{
	for (const auto& pipeline : graphicsPipelines)
	{
		vkDestroyPipeline(device, pipeline, nullptr);
	}
	vkDestroyRenderPass(device, renderPass, nullptr);
}

void PipelineFactory::createRenderPass(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat) {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = depthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};
	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}

const std::vector<VkPipeline>& PipelineFactory::getVkPipelines() const {
	return graphicsPipelines;
}

const std::vector<std::unique_ptr<ENG::Pipeline>>& PipelineFactory::getEngPipelines() const {
	return eng_pipelines;
}

const VkRenderPass& PipelineFactory::getRenderPass() const {
	return renderPass;
}

const VkDescriptorSetLayout& PipelineFactory::getDescriptorSetLayout(const std::string& shader) const {
	const size_t idx = pipeline_names.at(shader);
	assert(idx < eng_pipelines.size());
	assert(eng_pipelines.at(idx) != nullptr);
	return eng_pipelines.at(idx)->getDescriptorSetLayout();
}

const VkPipeline& PipelineFactory::getVkPipeline(const std::string& shader) const {
	const size_t idx = pipeline_names.at(shader);
	assert(idx < graphicsPipelines.size());
	return graphicsPipelines.at(idx);
}

const VkPipelineLayout& PipelineFactory::getVkPipelineLayout(const std::string& shader) const {
	const size_t idx = pipeline_names.at(shader);
	assert(idx < eng_pipelines.size());
	assert(eng_pipelines.at(idx) != nullptr);
	return eng_pipelines.at(idx)->getPipelineLayout();
}
} // End namespace
