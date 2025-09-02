#ifndef ENG_PIPELINE_FACTORY_DEF
#define ENG_PIPELINE_FACTORY_DEF
#include<vector>
#include "vulkan/vulkan_core.h"
#include "pipelines/ENG_Pipeline.hpp"
namespace ENG {
class PipelineFactory {

public:
	PipelineFactory(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat);
	~PipelineFactory();
	const std::vector<VkPipeline>& getVkPipelines() const;
	const std::vector<ENG::Pipeline>& getEngPipelines() const;
private:
	std::vector<VkPipeline> graphicsPipelines;
	std::vector<ENG::Pipeline> eng_pipelines;
	const VkDevice& device;

	PipelineFactory() = delete;
	PipelineFactory(const PipelineFactory&) = delete;
	const PipelineFactory& operator=(const PipelineFactory&) = delete;
}; // End class
} // End namespace
#endif
