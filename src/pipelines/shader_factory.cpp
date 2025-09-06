#include "pipelines/shader_factory.hpp"
#include<fstream>
#include<filesystem>


static std::vector<char> readFile(const std::filesystem::path& filepath) {
	std::ifstream file(filepath.native(), std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t) file.tellg();
	std::vector<char> buffer(fileSize);
	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();
	
	return buffer;
}

VkShaderModule createShaderModule(const VkDevice& device, const std::vector<char>& code) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

VkPipelineShaderStageCreateInfo createDefaultStage(const VkShaderModule* module, const VkShaderStageFlagBits& stage_enum) {
	assert(module);
	VkPipelineShaderStageCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = stage_enum;
	info.module = *module;
	info.pName = "main";

	return info;
}

ShaderFactory::ShaderFactory(const VkDevice& device) : device(device) {
	for (size_t i = 0; i < filepaths.size(); ++i) {
		modules.push_back(createShaderModule(device, readFile(filepaths.at(i))));
	}

	assert(modules.size() == 4);
	stages.push_back(createDefaultStage(&modules.at(0), VK_SHADER_STAGE_VERTEX_BIT));
	stages.push_back(createDefaultStage(&modules.at(1), VK_SHADER_STAGE_FRAGMENT_BIT));
	stages.push_back(createDefaultStage(&modules.at(2), VK_SHADER_STAGE_VERTEX_BIT));
	stages.push_back(createDefaultStage(&modules.at(3), VK_SHADER_STAGE_FRAGMENT_BIT));

	assert(stages.size() == 4);
	module_map = {
		{ENG_SHADER::PosColTex, {&modules.at(0), &modules.at(1)}},
		{ENG_SHADER::PosNorTex, {&modules.at(2), &modules.at(3)}}
	};

	stage_map = {
		{ENG_SHADER::PosColTex, {&stages.at(0), &stages.at(1)}},
		{ENG_SHADER::PosNorTex, {&stages.at(2), &stages.at(3)}}
	};
}

ShaderFactory::~ShaderFactory() {
	for (auto& module : modules) {
		vkDestroyShaderModule(device, module, nullptr);
	}
}

const std::vector<VkPipelineShaderStageCreateInfo*>& ShaderFactory::get_shader_stages(const ENG_SHADER& shader) const {
	return stage_map.at(shader);
}

const std::vector<VkShaderModule*>& ShaderFactory::get_shader_modules(const ENG_SHADER& shader) const {
	return module_map.at(shader);
}
