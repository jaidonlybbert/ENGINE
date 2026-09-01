#pragma once
#include <vulkan/vulkan_core.h>

#include <string>
#include <vector>

class PipelineFactoryI {
   public:
    ~PipelineFactoryI() = default;

    virtual const VkDescriptorSetLayout& getDescriptorSetLayout(const std::string& shader) const = 0;
    virtual const std::vector<VkPipeline>& getVkPipelines() const = 0;
    virtual const VkRenderPass& getRenderPass() const = 0;
    virtual void initialize(const VkDevice device, const VkFormat& swapChainImageFormat,
                            const VkFormat& depthFormat) = 0;
    virtual void cleanup() = 0;
    virtual const VkPipeline& getVkPipeline(const std::string& shader) const = 0;
    virtual const VkPipelineLayout& getVkPipelineLayout(const std::string& shader) const = 0;
};
