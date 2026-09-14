#include "filesystem/android/AndroidAssetProvider.hpp"

#include <android/asset_manager.h>

#include <stdexcept>

namespace ENG {

AndroidAssetProvider::AndroidAssetProvider(AAssetManager* assetManager) : assetManager(assetManager) {}

std::vector<char> AndroidAssetProvider::readBytes(const std::filesystem::path& logicalPath) const {
    // generic_string(), not string(): AAssetManager wants forward-slash-separated names
    // regardless of host OS, and std::filesystem::path::string() would render backslashes
    // if this were ever cross-compiled on/for a path convention that uses them.
    const std::string name = logicalPath.generic_string();
    AAsset* asset = AAssetManager_open(assetManager, name.c_str(), AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        throw std::runtime_error("failed to open Android asset: " + name);
    }

    const auto size = static_cast<size_t>(AAsset_getLength(asset));
    std::vector<char> buffer(size);
    AAsset_read(asset, buffer.data(), size);
    AAsset_close(asset);

    return buffer;
}

std::vector<char> AndroidAssetProvider::readShaderBytes(const std::string& shaderFileName) const {
    return readBytes(std::filesystem::path("shaders") / shaderFileName);
}

const std::filesystem::path& AndroidAssetProvider::getSpacefloorObj() const {
    static const std::filesystem::path path{"models/Spacefloor.obj"};
    return path;
}

const std::filesystem::path& AndroidAssetProvider::getSpacefloorObj2() const {
    static const std::filesystem::path path{"models/Spacefloor4.obj"};
    return path;
}

const std::filesystem::path& AndroidAssetProvider::getRoomObj() const {
    static const std::filesystem::path path{"models/viking_room.obj"};
    return path;
}

const std::filesystem::path& AndroidAssetProvider::getRoomTex() const {
    static const std::filesystem::path path{"textures/viking_room.png"};
    return path;
}

const std::filesystem::path& AndroidAssetProvider::getSpacefloorTex() const {
    static const std::filesystem::path path{"textures/Spacefloor.png"};
    return path;
}

const std::filesystem::path& AndroidAssetProvider::getGltfDir() const {
    static const std::filesystem::path path{"gltf/suzanne/suzanne.gltf"};
    return path;
}

const std::filesystem::path& AndroidAssetProvider::getMtlDir() const {
    static const std::filesystem::path path{"models"};
    return path;
}

}  // namespace ENG
