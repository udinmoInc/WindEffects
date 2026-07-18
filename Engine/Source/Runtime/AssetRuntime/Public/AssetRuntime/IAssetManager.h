#pragma once

#include "AssetCooker/WepakFormat.h"
#include "AssetImporter/AssetGuid.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/RuntimeTypes.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string_view>

namespace we::runtime::assetruntime {

struct ASSETRUNTIME_API AssetRuntimeDependencies {
    std::filesystem::path contentRoot;          // loose cooked assets
    std::filesystem::path packagePath;          // optional .wepak
    uint64_t memoryBudgetBytes = 512ull * 1024ull * 1024ull; // 512 MiB default
    unsigned workerThreads = 0;
    std::function<void(std::string_view)> onLog;
};

/// Runtime asset streaming: async load, refcount, lazy eviction under budget.
class ASSETRUNTIME_API IAssetManager {
public:
    virtual ~IAssetManager() = default;

    [[nodiscard]] virtual bool MountContent(const std::filesystem::path& contentRoot) = 0;
    [[nodiscard]] virtual bool MountPackage(const std::filesystem::path& wepakPath) = 0;

    /// Increment refcount; loads synchronously if not resident.
    [[nodiscard]] virtual std::shared_ptr<RuntimeAsset> Acquire(
        const we::runtime::assetimporter::AssetGuid& guid) = 0;

    /// Increment refcount; queue async load.
    [[nodiscard]] virtual we::runtime::assetimporter::AssetGuid AcquireAsync(
        const we::runtime::assetimporter::AssetGuid& guid,
        AssetLoadCallback onLoaded = {},
        AssetLoadProgressCallback onProgress = {}) = 0;

    /// Decrement refcount; may schedule eviction when zero and over budget.
    virtual void Release(const we::runtime::assetimporter::AssetGuid& guid) = 0;

    [[nodiscard]] virtual std::shared_ptr<RuntimeAsset> TryGet(
        const we::runtime::assetimporter::AssetGuid& guid) const = 0;

    [[nodiscard]] virtual bool LoadBundle(const AssetBundleDesc& bundle) = 0;

    [[nodiscard]] virtual uint64_t GetResidentBytes() const = 0;
    [[nodiscard]] virtual uint64_t GetMemoryBudgetBytes() const = 0;
    virtual void SetMemoryBudgetBytes(uint64_t bytes) = 0;

    /// Evict zero-refcount assets until under budget (LRU).
    virtual void EnforceBudget() = 0;

    virtual void WaitForIdle() = 0;
    virtual void Shutdown() = 0;
};

[[nodiscard]] ASSETRUNTIME_API std::unique_ptr<IAssetManager> CreateAssetManager(
    AssetRuntimeDependencies deps = {});

} // namespace we::runtime::assetruntime
