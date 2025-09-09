#include "pipelines/Pipeline.hpp"

class ShaderFactory;

namespace ENG {

class Pipeline_PosColTex : public Pipeline {
public:
	Pipeline_PosColTex(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
};

class Pipeline_PosNorTex : public Pipeline {
public:
	Pipeline_PosNorTex(const VkDevice& device, const VkFormat& swapChainImageFormat, const VkFormat& depthFormat,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
};

} // end namespace
