#ifndef ENG_SHADER_FACTORY_DEF
#define ENG_SHADER_FACTORY_DEF
#include "pipelines/utils.hpp"
#include "vulkan/vulkan_core.h"
#include "boost/filesystem/path.hpp"
#include<map>
#include<vector>
#include<assert.h>

enum class ENG_SHADER {
	PosColTex,
	PosNorTex
};

class ShaderFactory {
private:
	const VkDevice& device;
	static inline const std::vector<boost::filesystem::path> filepaths = {
		boost::filesystem::path("../shaders/posColTexVert.vert.spv"),
		boost::filesystem::path("../shaders/posColTexFrag.frag.spv"),
		boost::filesystem::path("../shaders/posNorTexVert.vert.spv"),
		boost::filesystem::path("../shaders/posNorTexFrag.frag.spv")
	};
	std::vector<VkShaderModule> modules;
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	std::map<ENG_SHADER, std::vector<VkShaderModule*>> module_map;
	std::map<ENG_SHADER, std::vector<VkPipelineShaderStageCreateInfo*>> stage_map;

public:
	explicit ShaderFactory(const VkDevice& device);
	~ShaderFactory();

	const std::vector<VkShaderModule*>& get_shader_modules(const ENG_SHADER& shader) const;
	const std::vector<VkPipelineShaderStageCreateInfo*>& get_shader_stages(const ENG_SHADER& shader) const;
};
#endif
