#include<vector>
#include<array>
#include "vulkan/vulkan_core.h"
#include "pipelines/PipelineFactory.hpp"
#include "pipelines/ShaderFactory.hpp"
#include "pipelines/Pipeline.hpp"
#include "pipelines/PipelinePosColTex.hpp"

namespace ENG {
PipelineFactory::PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat,
								 const VkFormat& depthFormat) : device(device) {
	std::vector<VkGraphicsPipelineCreateInfo> pipelineCreateInfos;
	const ShaderFactory& shader_factory{device};
	createRenderPass(device, swapChainImageFormat, depthFormat);

	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosColTex>(device, renderPass, shader_factory, pipelineCreateInfos));
	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosNorTex>(device, renderPass, shader_factory, pipelineCreateInfos));

	// TODO: Graphics pipelines is more than 1!
	assert(pipelineCreateInfos.size() == 2);
	graphicsPipelines.resize(pipelineCreateInfos.size());
	assert(graphicsPipelines.data() && pipelineCreateInfos.data());
	const auto* pCreateInfos = pipelineCreateInfos.data();
	auto* pPipelines = graphicsPipelines.data();
	const auto success = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 2, pCreateInfos, nullptr, pPipelines);
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

const VkDescriptorSetLayout& PipelineFactory::getDescriptorSetLayout(const ENG_SHADER shader) const {
	return eng_pipelines.at(static_cast<size_t>(shader))->getDescriptorSetLayout();
}

const VkPipeline& PipelineFactory::getVkPipeline(const ENG_SHADER shader) const {
	return graphicsPipelines.at(static_cast<size_t>(shader));
}

const VkPipelineLayout& PipelineFactory::getVkPipelineLayout(const ENG_SHADER shader) const {
	return eng_pipelines.at(static_cast<size_t>(shader))->getPipelineLayout();
}
} // End namespace
