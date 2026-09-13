#pragma once
#include "filesystem/AssetProviderI.hpp"

namespace ENG {

// Desktop AssetProviderI backend: reads assets from the folders installed alongside the
// Engine executable (see EngineConfig.hpp's Engine_INSTALL_DIR).
class LocalAssetProvider : public AssetProviderI {
   public:
    std::vector<char> readShaderBytes(const std::string& shaderFileName) const override;

    const std::filesystem::path& getGltfDir() const override;
    const std::filesystem::path& getMtlDir() const override;
    const std::filesystem::path& getRoomObj() const override;
    const std::filesystem::path& getRoomTex() const override;
    const std::filesystem::path& getSpacefloorObj() const override;
    const std::filesystem::path& getSpacefloorObj2() const override;
    const std::filesystem::path& getSpacefloorTex() const override;
};

}  // namespace ENG
