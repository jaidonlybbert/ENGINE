#ifndef ENG_PIPELINE_FACTORY_DEF
#define ENG_PIPELINE_FACTORY_DEF
#include<vector>
#include<memory>
#include "vulkan/vulkan_core.h"
#include "renderer/pipelines/Pipeline.hpp"
#include "renderer/pipelines/ShaderFactory.hpp"

namespace ENG {
class PipelineFactory {

public:
	PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat);
	~PipelineFactory();
	void createRenderPass(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat);
	const std::vector<VkPipeline>& getVkPipelines() const;
	const std::vector<std::unique_ptr<ENG::Pipeline>>& getEngPipelines() const;
	const VkRenderPass& getRenderPass() const;
	const VkDescriptorSetLayout& getDescriptorSetLayout(const ENG_SHADER shader) const;
	const VkPipeline& getVkPipeline(const ENG_SHADER shader) const;
	const VkPipelineLayout& getVkPipelineLayout(const ENG_SHADER shader) const;
private:
	std::vector<VkPipeline> graphicsPipelines;
	std::vector<std::unique_ptr<ENG::Pipeline>> eng_pipelines;
	const VkDevice& device;
	VkRenderPass renderPass;

	PipelineFactory() = delete;
	PipelineFactory(const PipelineFactory&) = delete;
	const PipelineFactory& operator=(const PipelineFactory&) = delete;
}; // End class
} // End namespace
#endif
