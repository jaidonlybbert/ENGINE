#include <cassert>

#include "filesystem/AssetProviderI.hpp"

namespace ENG {

namespace {
AssetProviderI* activeProvider = nullptr;
}  // namespace

AssetProviderI& getAssetProvider() {
    assert(activeProvider != nullptr && "setAssetProvider() must be called before getAssetProvider()");
    return *activeProvider;
}

void setAssetProvider(AssetProviderI& provider) { activeProvider = &provider; }

}  // namespace ENG
