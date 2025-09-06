#ifndef ENG_SHADER_FACTORY_DEF
#define ENG_SHADER_FACTORY_DEF
#include "pipelines/utils.hpp"
#include "vulkan/vulkan_core.h"
#include "EngineConfig.h"
#include<map>
#include<vector>
#include<assert.h>
#include<filesystem>

enum class ENG_SHADER {
	PosColTex,
	PosNorTex
};

static const std::filesystem::path& install_dir{Engine_INSTALL_DIR};

class ShaderFactory {
private:
	const VkDevice& device;
	static inline const std::vector<std::filesystem::path> filepaths = {
		install_dir / "shaders" / "posColTexVert.vert.spv",
		install_dir / "shaders" / "posColTexFrag.frag.spv",
		install_dir / "shaders" / "posNorTexVert.vert.spv",
		install_dir / "shaders" / "posNorTexFrag.frag.spv"
	};
	std::vector<VkShaderModule> modules;
	std::vector<VkPipelineShaderStageCreateInfo> stages;
	std::map<ENG_SHADER, std::vector<VkShaderModule*>> module_map;
	std::map<ENG_SHADER, std::vector<VkPipelineShaderStageCreateInfo*>> stage_map;

public:
	ShaderFactory(const VkDevice& device);
	~ShaderFactory();

	const std::vector<VkShaderModule*>& get_shader_modules(const ENG_SHADER& shader) const;
	const std::vector<VkPipelineShaderStageCreateInfo*>& get_shader_stages(const ENG_SHADER& shader) const;
};
#endif
