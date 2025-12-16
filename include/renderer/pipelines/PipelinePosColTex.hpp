#ifndef ENG_PIPELINE_POSCOLTEX
#define ENG_PIPELINE_POSCOLTEX
#include "renderer/pipelines/Pipeline.hpp"

class ShaderFactory;

namespace ENG {

class Pipeline_PosColTex : public Pipeline {
public:
	Pipeline_PosColTex(const VkDevice& device, const VkRenderPass& renderPass,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
};

class Pipeline_PosNorTex : public Pipeline {
public:
	Pipeline_PosNorTex(const VkDevice& device, const VkRenderPass& renderPass,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
};

class Pipeline_PosBB : public Pipeline {
public:
	Pipeline_PosBB(const VkDevice& device, const VkRenderPass& renderPass,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
	void createDescriptorSetLayout(const VkDevice& device) override;
	void createInputAssemblyInfo() override;
	void createRasterizationStateInfo() override;
};

class Pipeline_PosNorCol : public Pipeline {
public:
	Pipeline_PosNorCol(const VkDevice& device, const VkRenderPass& renderpass,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
	void createDescriptorSetLayout(const VkDevice& device) override;
};

class Pipeline_Goldberg : public Pipeline {
public:
	Pipeline_Goldberg(const VkDevice& device, const VkRenderPass& renderpass,
		const ShaderFactory& shader_fac, std::vector<VkGraphicsPipelineCreateInfo>& pipelineCreateInfos);
	void createShaderStages(const ShaderFactory& shader_fac) override;
	void createVertexInputInfo() override;
	void createDescriptorSetLayout(const VkDevice& device) override;
};
} // end namespace
#endif
