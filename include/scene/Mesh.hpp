#ifndef ENG_MESH_DEF
#define ENG_MESH_DEF

#include <cstring>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "scene/Primitives.hpp"

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
