#pragma once
#include <vector>

#include "vulkan/vulkan_core.h"

class MeshAdapter {
   public:
    template <typename T>
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(T);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    template <typename T>
    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};
