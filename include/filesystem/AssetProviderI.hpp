#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace ENG {

// Abstracts asset access so callers don't assume a folder-based install layout - Android
// has no such filesystem, assets ship inside the APK and are read through AAssetManager
// instead (see issue #42). LocalAssetProvider (filesystem/LocalAssetProvider.hpp) is the
// only implementation today, wrapping the current install-directory layout.
//
// Shader bytes are the one asset type the engine reads directly, so they're the one
// fully ported to a byte-buffer accessor here. Models/textures still go through
// tinyobjloader/tinygltf/stb_image, which do their own file I/O against a path and
// aren't routed through this interface yet - a real Android backend would also need
// those libraries' own custom file-loading hooks (tinygltf's FsCallbacks,
// tinyobjloader's material reader callback, stbi_load_from_memory), which is separate,
// larger follow-up work.
class AssetProviderI {
   public:
    virtual ~AssetProviderI() = default;

    virtual std::vector<char> readShaderBytes(const std::string& shaderFileName) const = 0;

    virtual const std::filesystem::path& getGltfDir() const = 0;
    virtual const std::filesystem::path& getMtlDir() const = 0;
    virtual const std::filesystem::path& getRoomObj() const = 0;
    virtual const std::filesystem::path& getRoomTex() const = 0;
    virtual const std::filesystem::path& getSpacefloorObj() const = 0;
    virtual const std::filesystem::path& getSpacefloorObj2() const = 0;
    virtual const std::filesystem::path& getSpacefloorTex() const = 0;
};

// Global accessor, set once at startup (see main.cpp). Asset lookups happen in ~10 call
// sites spread across independently-buildable libraries (scene, renderer, scenes/world -
// see issue #42's scene-library split), too many and too spread out for constructor
// injection to be practical the way it is for RenderAdapterI/PipelineFactoryI (each of
// which has exactly one call site) - a global service locator is the standard answer for
// exactly this shape of problem, and matches this codebase's existing globals for
// similarly cross-cutting concerns (the ENG_LOG_* logger, Application::io_ctx).
AssetProviderI& getAssetProvider();
void setAssetProvider(AssetProviderI& provider);

}  // namespace ENG
