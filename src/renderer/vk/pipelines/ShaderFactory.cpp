#include "renderer/vk/pipelines/ShaderFactory.hpp"

#include <assert.h>

#include "filesystem/AssetProviderI.hpp"
#include "logger/Logging.hpp"

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

VkPipelineShaderStageCreateInfo createDefaultStage(const VkShaderModule* module,
                                                   const VkShaderStageFlagBits& stage_enum) {
    assert(module);
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage_enum;
    info.module = *module;
    info.pName = "main";

    return info;
}

ShaderFactory::ShaderFactory(const VkDevice& device) : device(device) {
    static const std::vector<std::string> shaderFileNames = {
        "posColTexVert.vert.spv", "posColTexFrag.frag.spv", "posNorTexVert.vert.spv", "posNorTexFrag.frag.spv",
        "posBBVert.vert.spv",     "posBBFrag.frag.spv",     "posNorColVert.vert.spv", "posNorColFrag.frag.spv",
        "goldbergVert.vert.spv",  "goldbergFrag.frag.spv",
    };

    auto& assetProvider = ENG::getAssetProvider();
    for (const auto& shaderFileName : shaderFileNames) {
        ENG_LOG_DEBUG("Creating module for " << shaderFileName);
        modules.push_back(createShaderModule(device, assetProvider.readShaderBytes(shaderFileName)));
    }

    assert(modules.size() == 10);
    stages.push_back(createDefaultStage(&modules.at(0), VK_SHADER_STAGE_VERTEX_BIT));
    stages.push_back(createDefaultStage(&modules.at(1), VK_SHADER_STAGE_FRAGMENT_BIT));
    stages.push_back(createDefaultStage(&modules.at(2), VK_SHADER_STAGE_VERTEX_BIT));
    stages.push_back(createDefaultStage(&modules.at(3), VK_SHADER_STAGE_FRAGMENT_BIT));
    stages.push_back(createDefaultStage(&modules.at(4), VK_SHADER_STAGE_VERTEX_BIT));
    stages.push_back(createDefaultStage(&modules.at(5), VK_SHADER_STAGE_FRAGMENT_BIT));
    stages.push_back(createDefaultStage(&modules.at(6), VK_SHADER_STAGE_VERTEX_BIT));
    stages.push_back(createDefaultStage(&modules.at(7), VK_SHADER_STAGE_FRAGMENT_BIT));
    stages.push_back(createDefaultStage(&modules.at(8), VK_SHADER_STAGE_VERTEX_BIT));
    stages.push_back(createDefaultStage(&modules.at(9), VK_SHADER_STAGE_FRAGMENT_BIT));

    assert(stages.size() == 10);
    module_map = {{"PosColTex", {&modules.at(0), &modules.at(1)}},
                  {"PosNorTex", {&modules.at(2), &modules.at(3)}},
                  {"PosBB", {&modules.at(4), &modules.at(5)}},
                  {"PosNorCol", {&modules.at(6), &modules.at(7)}},
                  {"Goldberg", {&modules.at(8), &modules.at(9)}}};

    stage_map = {{"PosColTex", {&stages.at(0), &stages.at(1)}},
                 {"PosNorTex", {&stages.at(2), &stages.at(3)}},
                 {"PosBB", {&stages.at(4), &stages.at(5)}},
                 {"PosNorCol", {&stages.at(6), &stages.at(7)}},
                 {"Goldberg", {&stages.at(8), &stages.at(9)}}};

    ENG_LOG_DEBUG("Loaded all shader modules");
}

ShaderFactory::~ShaderFactory() {
    for (auto& module : modules) {
        vkDestroyShaderModule(device, module, nullptr);
    }
}

const std::vector<VkPipelineShaderStageCreateInfo*>& ShaderFactory::get_shader_stages(const std::string& shader) const {
    return stage_map.at(shader);
}

const std::vector<VkShaderModule*>& ShaderFactory::get_shader_modules(const std::string& shader) const {
    return module_map.at(shader);
}
