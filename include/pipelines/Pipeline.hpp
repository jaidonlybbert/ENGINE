#ifndef ENG_PIPELINE_DEF
#define ENG_PIPELINE_DEF
#include<vulkan/vulkan_core.h>
#include<vector>

class ShaderFactory;
namespace ENG {
class Pipeline {
public:
	explicit Pipeline(const VkDevice& device);
	virtual ~Pipeline();
	void Initialize(const VkRenderPass& renderPass,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	const VkDescriptorSetLayout& getDescriptorSetLayout() const;
	const VkPipelineLayout& getPipelineLayout() const;

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
	VkPushConstantRange pushConstantRange{};
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	VkPipelineLayout pipelineLayout{};
	VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
	VkRenderPass renderPass{};

	virtual void createShaderStages(const ShaderFactory &shader_fac);
	virtual void createDynamicStateInfo();
	virtual void createVertexInputInfo();
	virtual void createInputAssemblyInfo();
	virtual void createViewportStateInfo();
	virtual void createRasterizationStateInfo();
	virtual void createMultisamplingStateInfo();
	virtual void createColorBlendAttachmentState();
	virtual void createColorBlendingStateInfo();
	virtual void createDescriptorSetLayout(const VkDevice& device);
	virtual void createPushConstantsRange();
	virtual void createPipelineLayoutInfo();
	virtual void createDepthStencilInfo();
	virtual void createPipelineInfo(VkGraphicsPipelineCreateInfo& create_info, const VkRenderPass& renderPass);
}; // End class
} // end namespace
#endif
