#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/Types.h"
#include "AssetRuntime/AssetHandle.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/VirtualPath.h"

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

enum class AssetStreamPriority : int32_t {
    Lowest = 0,
    Low = 25,
    Normal = 50,
    High = 75,
    Critical = 100
};

enum class AssetResidencyPolicy : uint32_t {
    /// Load on first Acquire; eligible for eviction when refcount is zero.
    Lazy = 0,
    /// Keep resident while mounted unless explicitly unloaded.
    Sticky,
    /// Prefetch dependencies eagerly on Acquire.
    PreloadDependencies
};

/// Runtime-owned cooked asset record. Consumers hold AssetHandle, not raw pointers.
struct ASSETRUNTIME_API RuntimeAsset {
    we::runtime::assetimporter::AssetGuid guid{};
    VirtualPath virtualPath;
    we::runtime::assetimporter::AssetKind kind = we::runtime::assetimporter::AssetKind::Unknown;
    std::string displayName;
    std::string mountId;
    std::string contentType;
    std::vector<std::byte> payload;
    std::vector<we::runtime::assetimporter::AssetGuid> dependencies;
    /// Optional decoded domain object produced by a registered IAssetLoader.
    std::shared_ptr<void> decodedObject;
    std::string decodedTypeId;
    uint64_t residentBytes = 0;
    uint64_t lastLoadMicroseconds = 0;
    AssetLoadState state = AssetLoadState::Unloaded;
    uint32_t refCount = 0;
    AssetHandle handle{};
};

struct ASSETRUNTIME_API AssetBundleDesc {
    std::string name;
    std::vector<we::runtime::assetimporter::AssetGuid> assets;
    std::vector<VirtualPath> paths;
    bool loadDependencies = true;
    AssetStreamPriority priority = AssetStreamPriority::Normal;
};

struct ASSETRUNTIME_API AssetLoadProgress {
    we::runtime::assetimporter::AssetGuid guid{};
    AssetHandle handle{};
    StreamRequestId requestId{};
    AssetLoadState state = AssetLoadState::Queued;
    float fraction = 0.0f;
    std::string stage;
};

using AssetLoadCallback = std::function<void(AssetHandle, std::shared_ptr<RuntimeAsset>)>;
using AssetLoadProgressCallback = std::function<void(const AssetLoadProgress&)>;
using AssetHotReloadCallback = std::function<void(AssetHandle, std::shared_ptr<RuntimeAsset>)>;

struct ASSETRUNTIME_API StreamRequestDesc {
    we::runtime::assetimporter::AssetGuid guid{};
    VirtualPath path;
    AssetStreamPriority priority = AssetStreamPriority::Normal;
    bool loadDependencies = true;
    AssetLoadCallback onLoaded;
    AssetLoadProgressCallback onProgress;
};

struct ASSETRUNTIME_API PackageMountRequest {
    std::string mountId;                      // unique id, e.g. "BaseGame", "DLC1"
    std::filesystem::path source;             // .wepak or cooked directory
    std::string virtualRoot = "/Game";        // assets appear under this root
    int priority = 0;                         // higher wins on GUID/path collisions
    uint32_t expectedVersion = 0;             // 0 = accept any
    std::string expectedPackageId;            // empty = accept any
    bool readOnly = true;
};

struct ASSETRUNTIME_API PackageMountResult {
    bool success = false;
    std::string errorCode;
    std::string errorMessage;
    std::string mountId;
    uint32_t packageVersion = 0;
    uint64_t assetCount = 0;
};
/// Returns true if extension is a cooked engine asset (never raw PNG/FBX/WAV/…).
[[nodiscard]] ASSETRUNTIME_API bool IsCookedAssetExtension(std::string_view extension);

/// Returns true if extension is a forbidden raw source format at runtime.
[[nodiscard]] ASSETRUNTIME_API bool IsRawSourceExtension(std::string_view extension);

} // namespace we::runtime::assetruntime
