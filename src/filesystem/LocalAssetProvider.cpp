#include "filesystem/LocalAssetProvider.hpp"

#include <fstream>
#include <stdexcept>

#include "EngineConfig.hpp"

namespace {

const std::filesystem::path& get_install_dir() {
    static const std::filesystem::path& install_dir{Engine_INSTALL_DIR};
    return install_dir;
}

}  // namespace

namespace ENG {

std::vector<char> LocalAssetProvider::readShaderBytes(const std::string& shaderFileName) const {
    const auto path = get_install_dir() / "shaders" / shaderFileName;
    std::ifstream file(path.native(), std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open shader file: " + path.string());
    }

    const size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);

    return buffer;
}

const std::filesystem::path& LocalAssetProvider::getSpacefloorObj() const {
    static const std::filesystem::path& model_path{get_install_dir() / "models" / "Spacefloor.obj"};
    return model_path;
}

const std::filesystem::path& LocalAssetProvider::getSpacefloorObj2() const {
    static const std::filesystem::path& model_path{get_install_dir() / "models" / "Spacefloor4.obj"};
    return model_path;
}

const std::filesystem::path& LocalAssetProvider::getRoomObj() const {
    static const std::filesystem::path& model_path{get_install_dir() / "models" / "viking_room.obj"};
    return model_path;
}

const std::filesystem::path& LocalAssetProvider::getRoomTex() const {
    static const std::filesystem::path& tex_path{get_install_dir() / "textures" / "viking_room.png"};
    return tex_path;
}

const std::filesystem::path& LocalAssetProvider::getSpacefloorTex() const {
    static const std::filesystem::path& tex_path{get_install_dir() / "textures" / "Spacefloor.png"};
    return tex_path;
}

const std::filesystem::path& LocalAssetProvider::getGltfDir() const {
    static const std::filesystem::path& gltf_dir{get_install_dir() / "gltf" / "suzanne" / "suzanne.gltf"};
    return gltf_dir;
}

const std::filesystem::path& LocalAssetProvider::getMtlDir() const {
    static const std::filesystem::path& mtl_dir{get_install_dir() / "models"};
    return mtl_dir;
}

}  // namespace ENG
