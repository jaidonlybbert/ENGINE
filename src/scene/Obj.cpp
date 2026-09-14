#include "scene/Obj.hpp"

#include <sstream>
#include <vector>

#include "filesystem/AssetProviderI.hpp"
#include "logger/Logging.hpp"
#include "scene/Mesh.hpp"
#include "scene/Scene.hpp"
#include "tiny_obj_loader.h"

namespace {

// Reads .mtl material data through AssetProviderI instead of tinyobjloader's own built-in
// MaterialFileReader, which opens mtl_basedir/matId directly off a real filesystem - no
// such thing on Android (see issue #50). Unlike MaterialStreamReader (tinyobjloader's
// other built-in reader), this actually uses matId - each material file tinyobjloader
// asks for is fetched on demand, rather than requiring the caller to already know which
// single .mtl to preload.
class AssetProviderMaterialReader : public tinyobj::MaterialReader {
   public:
    explicit AssetProviderMaterialReader(std::filesystem::path mtlBaseDir) : mtlBaseDir(std::move(mtlBaseDir)) {}

    bool operator()(const std::string& matId, std::vector<tinyobj::material_t>* materials,
                    std::map<std::string, int>* matMap, std::string* warn, std::string* err) override {
        try {
            const auto bytes = ENG::getAssetProvider().readBytes(mtlBaseDir / matId);
            std::string content(bytes.begin(), bytes.end());
            std::istringstream stream(content);
            tinyobj::LoadMtl(matMap, materials, &stream, warn, err);
            return true;
        } catch (const std::exception& e) {
            if (warn) *warn += "Material file [" + matId + "] in " + mtlBaseDir.string() + ": " + e.what() + "\n";
            return false;
        }
    }

   private:
    std::filesystem::path mtlBaseDir;
};

}  // namespace

namespace ENG {

void loadModel(std::string name, const std::filesystem::path& objPath, const std::filesystem::path& texturePath,
               SceneState& sceneState, Node& attachmentPoint) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    auto& assetProvider = getAssetProvider();

    const auto objBytes = assetProvider.readBytes(objPath);
    std::string objContent(objBytes.begin(), objBytes.end());
    std::istringstream objStream(objContent);
    AssetProviderMaterialReader materialReader(assetProvider.getMtlDir());

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &objStream, &materialReader)) {
        throw std::runtime_error(warn + err);
    }

    auto texPath = texturePath;

    ENG_LOG_TRACE("Found " << materials.size() << " materials.");
    for (const auto& mat : materials) {
        ENG_LOG_TRACE("\tName: " << mat.name);
        ENG_LOG_TRACE("\tDiffuse texture: " << mat.diffuse_texname);
        texPath = (assetProvider.getMtlDir() / std::filesystem::path(mat.diffuse_texname)).lexically_normal();
        ENG_LOG_TRACE("\tConcat path: " << texPath);
    }

    std::unordered_map<std::filesystem::path, std::vector<VertexPosColTex>> vertices;
    std::unordered_map<std::filesystem::path, std::vector<uint32_t>> indices;
    auto idx = 0;
    for (const auto& shape : shapes) {
        auto& newNode = sceneState.graph.create_node();
        newNode.name = name + "-" + std::to_string(idx++);
        newNode.parent = &attachmentPoint;
        attachmentPoint.children.push_back(&newNode);

        // assumes material id is always the same per shape
        if (shape.mesh.material_ids.empty()) {
            const auto& shape_mat_id = shape.mesh.material_ids.at(0);
            const auto& shape_material = materials.at(shape_mat_id);
            texPath =
                (assetProvider.getMtlDir() / std::filesystem::path(shape_material.diffuse_texname)).lexically_normal();
            ENG_LOG_TRACE("Overwrite texture path with material found for mesh: " << texPath);
        }

        if (!vertices.contains(texPath)) {
            vertices.insert({texPath, {}});
            indices.insert({texPath, {}});
        }

        for (const auto& index : shape.mesh.indices) {
            VertexPosColTex vertex{};

            vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
                          attrib.vertices[3 * index.vertex_index + 2]};

            vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                               1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

            vertex.color = {1.0f, 1.0f, 1.0f};

            vertices.at(texPath).push_back(vertex);
            indices.at(texPath).push_back(indices.at(texPath).size());
        }

        sceneState.hostMeshDataBindQueue.push(BindHostMeshDataEvent{
            HostMeshData{std::move(vertices.at(texPath)), std::move(indices.at(texPath)), "PosColTex", texPath},
            newNode.nodeId});
    }
}

}  // namespace ENG
