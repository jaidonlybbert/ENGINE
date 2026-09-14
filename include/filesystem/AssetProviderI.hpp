#pragma once
#include <filesystem>
#include <string>
#include <vector>

namespace ENG {

// Abstracts asset access so callers don't assume a folder-based install layout - Android
// has no such filesystem, assets ship inside the APK and are read through AAssetManager
// instead (see issue #42). LocalAssetProvider (filesystem/LocalAssetProvider.hpp) and
// AndroidAssetProvider (filesystem/android/AndroidAssetProvider.hpp) are the two
// implementations (see issue #50).
//
// The get*() methods below name *which* asset something is - used as stable identifiers
// (e.g. VkRenderer's texture caches are keyed by the path getRoomTex() returns), not
// necessarily a real filesystem location. readBytes() is the actual mechanism for turning
// one of those identifiers (or any other logical path shaped the same way, e.g. a
// material filename tinyobjloader discovers while parsing an .obj it's already reading
// through this interface) into bytes - LocalAssetProvider resolves it against the
// installed asset folders like it always has; AndroidAssetProvider resolves it as an
// AAssetManager asset name instead. readShaderBytes() predates this and is kept as its
// own method (call sites want "give me this shader" without knowing the "shaders/"
// convention), but both implementations just forward it to readBytes() now.
class AssetProviderI {
   public:
    virtual ~AssetProviderI() = default;

    virtual std::vector<char> readShaderBytes(const std::string& shaderFileName) const = 0;

    // Throws if the asset can't be read. logicalPath is typically one of the get*()
    // results below, or a path built from one (e.g. getMtlDir() / "<material>.mtl").
    virtual std::vector<char> readBytes(const std::filesystem::path& logicalPath) const = 0;

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
