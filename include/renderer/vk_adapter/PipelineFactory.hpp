#ifndef ENG_PIPELINE_FACTORY_DEF
#define ENG_PIPELINE_FACTORY_DEF
#include <memory>
#include <vector>

#include "renderer/vk/pipelines/Pipeline.hpp"
#include "renderer/vk/pipelines/PipelineFactoryI.hpp"
#include "renderer/vk/pipelines/ShaderFactory.hpp"
#include "vulkan/vulkan_core.h"

namespace ENG {
class PipelineFactory : public PipelineFactoryI {
   public:
    PipelineFactory();
    ~PipelineFactory();

    // overrides
    const VkDescriptorSetLayout& getDescriptorSetLayout(const std::string& shader) const override;
    const std::vector<VkPipeline>& getVkPipelines() const override;
    const VkRenderPass& getRenderPass() const override;
    void initialize(const VkDevice device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat) override;
    void cleanup() override;
    const VkPipeline& getVkPipeline(const std::string& shader) const override;
    const VkPipelineLayout& getVkPipelineLayout(const std::string& shader) const override;

    void createRenderPass(const VkDevice device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat);
    const std::vector<std::unique_ptr<ENG::Pipeline>>& getEngPipelines() const;

   private:
    VkDevice device;
    std::map<std::string, size_t> pipeline_names;
    std::vector<VkPipeline> graphicsPipelines;
    std::vector<std::unique_ptr<ENG::Pipeline>> eng_pipelines;
    VkRenderPass renderPass;

    PipelineFactory(const PipelineFactory&) = delete;
    const PipelineFactory& operator=(const PipelineFactory&) = delete;
};  // End class
}  // namespace ENG
#endif
