#ifndef ENG_SHADER_FACTORY_DEF
#define ENG_SHADER_FACTORY_DEF
#include <map>
#include <string>
#include <vector>

#include "vulkan/vulkan_core.h"

class ShaderFactory {
   private:
    const VkDevice& device;
    std::vector<VkShaderModule> modules;
    std::vector<VkPipelineShaderStageCreateInfo> stages;
    std::map<std::string, std::vector<VkShaderModule*>> module_map;
    std::map<std::string, std::vector<VkPipelineShaderStageCreateInfo*>> stage_map;

   public:
    ShaderFactory(const VkDevice& device);
    ~ShaderFactory();

    const std::vector<VkShaderModule*>& get_shader_modules(const std::string& shader) const;
    const std::vector<VkPipelineShaderStageCreateInfo*>& get_shader_stages(const std::string& shader) const;
};
#endif
