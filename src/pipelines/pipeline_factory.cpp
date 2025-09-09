#include<vector>
#include "vulkan/vulkan_core.h"
#include "pipelines/pipeline_factory.hpp"
#include "pipelines/shader_factory.hpp"
#include "pipelines/Pipeline.hpp"
#include "pipelines/Pipeline_PosColTex.hpp"

namespace ENG {
PipelineFactory::PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat,
								 const VkFormat& depthFormat) : device(device) {
	std::vector<VkGraphicsPipelineCreateInfo> pipelineCreateInfos;
	const ShaderFactory& shader_factory{device};

	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosColTex>(device, swapChainImageFormat, depthFormat, shader_factory, pipelineCreateInfos));
	eng_pipelines.emplace_back(std::make_unique<Pipeline_PosNorTex>(device, swapChainImageFormat, depthFormat, shader_factory, pipelineCreateInfos));

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
}

const std::vector<VkPipeline>& PipelineFactory::getVkPipelines() const {
	return graphicsPipelines;
}

const std::vector<std::unique_ptr<ENG::Pipeline>>& PipelineFactory::getEngPipelines() const {
	return eng_pipelines;
}

const VkRenderPass& PipelineFactory::getRenderPass(const ENG_SHADER shader) const {
	return eng_pipelines.at(static_cast<size_t>(shader))->getRenderPass();
}

const VkDescriptorSetLayout& PipelineFactory::getDescriptorSetLayout(const ENG_SHADER shader) const {
	return eng_pipelines.at(static_cast<size_t>(shader))->getDescriptorSetLayout();
}

const VkPipeline& PipelineFactory::getVkPipelines(const ENG_SHADER shader) const {
	return graphicsPipelines.at(static_cast<size_t>(shader));
}

const VkPipelineLayout& PipelineFactory::getVkPipelineLayout(const ENG_SHADER shader) const {
	return eng_pipelines.at(static_cast<size_t>(shader))->getPipelineLayout();
}
} // End namespace
