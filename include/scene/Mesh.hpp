#ifndef ENG_MESH_DEF
#define ENG_MESH_DEF

#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "glm/glm.hpp"
#include "logger/Logging.hpp"
#include "renderer/vk/Buffer.hpp"
#include "renderer/vk/Command.hpp"
#include "scene/Primitives.hpp"
#include "vulkan/vulkan_core.h"

namespace ENG {
using VertexT = std::variant<std::vector<VertexPosColTex>, std::vector<VertexPosNorCol>, std::vector<VertexPosNorTex>,
                             std::vector<VertexPos>>;

struct HostMeshData {
    VertexT vertexBuffer;
    std::vector<uint32_t> indexBuffer;
    std::string shaderId;
    std::optional<std::filesystem::path> texturePath;
};

struct BindHostMeshDataEvent {
    HostMeshData meshData;
    uint32_t nodeId;
};

class Mesh {};

}  // namespace ENG

#endif
