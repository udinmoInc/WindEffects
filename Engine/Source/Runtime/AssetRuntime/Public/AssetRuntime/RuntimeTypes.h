#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/Types.h"
#include "AssetRuntime/Export.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace we::runtime::assetruntime {

enum class AssetLoadState : uint32_t {
    Unloaded = 0,
    Queued,
    Loading,
    Loaded,
    Failed,
    Evicted
};

struct ASSETRUNTIME_API RuntimeAsset {
    we::runtime::assetimporter::AssetGuid guid{};
    we::runtime::assetimporter::AssetKind kind = we::runtime::assetimporter::AssetKind::Unknown;
    std::string displayName;
    std::filesystem::path sourcePath; // disk path or wepak relative
    std::vector<std::byte> payload;
    std::string contentType;
    uint64_t residentBytes = 0;
    AssetLoadState state = AssetLoadState::Unloaded;
    uint32_t refCount = 0;
};

struct ASSETRUNTIME_API AssetBundleDesc {
    std::string name;
    std::vector<we::runtime::assetimporter::AssetGuid> assets;
    bool loadDependencies = true;
};

struct ASSETRUNTIME_API AssetLoadProgress {
    we::runtime::assetimporter::AssetGuid guid{};
    AssetLoadState state = AssetLoadState::Queued;
    float fraction = 0.0f;
};

using AssetLoadCallback = std::function<void(std::shared_ptr<RuntimeAsset>)>;
using AssetLoadProgressCallback = std::function<void(const AssetLoadProgress&)>;

[[nodiscard]] ASSETRUNTIME_API bool IsCookedAssetExtension(std::string_view extension);
[[nodiscard]] ASSETRUNTIME_API bool IsRawSourceExtension(std::string_view extension);

} // namespace we::runtime::assetruntime
