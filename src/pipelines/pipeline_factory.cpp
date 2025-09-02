#include<vector>
#include "vulkan/vulkan_core.h"
#include "pipelines/pipeline_factory.hpp"
#include "pipelines/shader_factory.hpp"
#include "pipelines/ENG_Pipeline.hpp"

namespace ENG {
PipelineFactory::PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat,
								 const VkFormat& depthFormat) : device(device) {
	std::vector<VkGraphicsPipelineCreateInfo> pipelineCreateInfos;
	const ShaderFactory& shader_factory{device};

	eng_pipelines.emplace_back(device, swapChainImageFormat, depthFormat, shader_factory, pipelineCreateInfos);

	// TODO: Graphics pipelines is more than 1!
	assert(pipelineCreateInfos.size() == 1);
	graphicsPipelines.emplace_back();
	assert(graphicsPipelines.data() && pipelineCreateInfos.data());
	const auto* pCreateInfos = pipelineCreateInfos.data();
	auto* pPipelines = graphicsPipelines.data();
	const auto success = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, pCreateInfos, nullptr, pPipelines);
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
};

const std::vector<ENG::Pipeline>& PipelineFactory::getEngPipelines() const {
	return eng_pipelines;
};
} // End namespace
