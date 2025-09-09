#ifndef ENG_PIPELINE_FACTORY_DEF
#define ENG_PIPELINE_FACTORY_DEF
#include<vector>
#include<memory>
#include "vulkan/vulkan_core.h"
#include "pipelines/Pipeline.hpp"
#include "pipelines/shader_factory.hpp"

namespace ENG {
class PipelineFactory {

public:
	PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat);
	~PipelineFactory();
	const std::vector<VkPipeline>& getVkPipelines() const;
	const std::vector<std::unique_ptr<ENG::Pipeline>>& getEngPipelines() const;
	const VkRenderPass& getRenderPass(const ENG_SHADER shader) const;
	const VkDescriptorSetLayout& getDescriptorSetLayout(const ENG_SHADER shader) const;
	const VkPipeline& getVkPipelines(const ENG_SHADER shader) const;
	const VkPipelineLayout& getVkPipelineLayout(const ENG_SHADER shader) const;
private:
	std::vector<VkPipeline> graphicsPipelines;
	std::vector<std::unique_ptr<ENG::Pipeline>> eng_pipelines;
	const VkDevice& device;

	PipelineFactory() = delete;
	PipelineFactory(const PipelineFactory&) = delete;
	const PipelineFactory& operator=(const PipelineFactory&) = delete;
}; // End class
} // End namespace
#endif
