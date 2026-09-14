#pragma once
#include "filesystem/AssetProviderI.hpp"

struct AAssetManager;

namespace ENG {

// The Android AssetProviderI backend (see issue #50): reads assets packaged inside the
// APK via AAssetManager instead of a real filesystem. Its get*() results mirror
// LocalAssetProvider's same relative structure (e.g. "textures/viking_room.png") but
// without an install-dir prefix - AAssetManager_open() wants a bare name relative to the
// APK's assets/ root, not an absolute path.
class AndroidAssetProvider : public AssetProviderI {
   public:
    explicit AndroidAssetProvider(AAssetManager* assetManager);

    std::vector<char> readShaderBytes(const std::string& shaderFileName) const override;
    std::vector<char> readBytes(const std::filesystem::path& logicalPath) const override;

    const std::filesystem::path& getGltfDir() const override;
    const std::filesystem::path& getMtlDir() const override;
    const std::filesystem::path& getRoomObj() const override;
    const std::filesystem::path& getRoomTex() const override;
    const std::filesystem::path& getSpacefloorObj() const override;
    const std::filesystem::path& getSpacefloorObj2() const override;
    const std::filesystem::path& getSpacefloorTex() const override;

   private:
    AAssetManager* assetManager;
};

}  // namespace ENG
