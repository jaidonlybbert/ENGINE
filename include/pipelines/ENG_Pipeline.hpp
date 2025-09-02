#ifndef ENG_PIPELINE_DEF
#define ENG_PIPELINE_DEF
#include<vulkan/vulkan_core.h>
#include<vector>

class ShaderFactory;
namespace ENG {
class Pipeline {
public:
	explicit Pipeline(const VkDevice& device, const VkFormat& swapChainImageFormat,
		   const VkFormat& depthFormat, const ShaderFactory& shader_fac,
		   std::vector<VkGraphicsPipelineCreateInfo> &pipelineCreateInfos);
	virtual ~Pipeline();
	const VkRenderPass& getRenderPass() const;
	const VkDescriptorSetLayout& getDescriptorSetLayout() const;
	const VkPipelineLayout& getPipelineLayout() const;

private:
	const VkDevice& device;
	std::vector<VkPipelineShaderStageCreateInfo*> shader_stages;
	std::vector<VkDynamicState> dynamicStates;
	VkPipelineDynamicStateCreateInfo dynamicState{};
	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	VkVertexInputBindingDescription bindingDescription{};
	VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
	VkPipelineViewportStateCreateInfo viewportState{};
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
	VkPipelineRasterizationStateCreateInfo rasterizer{};
	VkPipelineMultisampleStateCreateInfo multisampling{};
	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	VkPipelineColorBlendStateCreateInfo colorBlending{};
	VkDescriptorSetLayout descriptorSetLayout{};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	VkPipelineLayout pipelineLayout{};
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	VkRenderPass renderPass{};

	virtual void createDynamicStateInfo();
	virtual void createVertexInputInfo();
	virtual void createInputAssemblyInfo();
	virtual void createViewportStateInfo();
	virtual void createRasterizationStateInfo();
	virtual void createMultisamplingStateInfo();
	virtual void createColorBlendAttachmentState();
	virtual void createColorBlendingStateInfo();
	virtual void createDescriptorSetLayout(const VkDevice& device);
	virtual void createPipelineLayoutInfo();
	virtual void createDepthStencilInfo();
	virtual void createRenderPass(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat);
	virtual void createPipelineInfo(VkGraphicsPipelineCreateInfo& create_info);
}; // End class
} // end namespace
#endif
