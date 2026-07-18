#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetRuntime/AssetDiagnostics.h"
#include "AssetRuntime/AssetHandle.h"
#include "AssetRuntime/AssetLoaderRegistry.h"
#include "AssetRuntime/Export.h"
#include "AssetRuntime/PackageProviderRegistry.h"
#include "AssetRuntime/RuntimeTypes.h"
#include "AssetRuntime/VirtualPath.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace we::runtime::assetruntime {

struct ASSETRUNTIME_API AssetRuntimeDependencies {
    std::filesystem::path contentRoot;  // optional initial loose cooked root
    std::filesystem::path packagePath;  // optional initial .wepak
    std::string contentVirtualRoot = "/Game";
    std::string packageMountId = "BasePackage";
    std::string contentMountId = "LooseContent";
    uint64_t memoryBudgetBytes = 512ull * 1024ull * 1024ull;
    unsigned workerThreads = 0;
    bool enableHotReload = false; // Editor / tools
    bool decodeOnLoad = true;     // invoke registered IAssetLoader when present
    std::function<void(std::string_view)> onLog;
    AssetHotReloadCallback onHotReload;
};

/// Single entry point for loading all engine assets at runtime.
/// Identifies assets by AssetGuid or VirtualPath — never by raw filesystem paths for Acquire.
class ASSETRUNTIME_API IAssetManager {
public:
    virtual ~IAssetManager() = default;

    // --- Package management (DLC / mods / patches without API changes) ---

    [[nodiscard]] virtual PackageMountResult MountPackage(const PackageMountRequest& request) = 0;
    [[nodiscard]] virtual bool UnmountPackage(std::string_view mountId) = 0;
    [[nodiscard]] virtual std::vector<PackageMountInfo> ListMounts() const = 0;

    /// Convenience: mount loose cooked directory (rejects raw source files).
    [[nodiscard]] virtual PackageMountResult MountContent(
        const std::filesystem::path& contentRoot,
        std::string_view mountId = "LooseContent",
        std::string_view virtualRoot = "/Game",
        int priority = 0) = 0;

    // --- Identity resolution ---

    [[nodiscard]] virtual std::optional<we::runtime::assetimporter::AssetGuid> ResolveGuid(
        const VirtualPath& path) const = 0;
    [[nodiscard]] virtual std::optional<VirtualPath> ResolvePath(
        const we::runtime::assetimporter::AssetGuid& guid) const = 0;
    [[nodiscard]] virtual bool Contains(
        const we::runtime::assetimporter::AssetGuid& guid) const = 0;
    [[nodiscard]] virtual bool Contains(const VirtualPath& path) const = 0;

    // --- Synchronous loading ---

    [[nodiscard]] virtual AssetHandle Acquire(const we::runtime::assetimporter::AssetGuid& guid) = 0;
    [[nodiscard]] virtual AssetHandle Acquire(const VirtualPath& path) = 0;

    /// Acquire without loading dependencies (still loads the asset itself).
    [[nodiscard]] virtual AssetHandle AcquireLazy(
        const we::runtime::assetimporter::AssetGuid& guid) = 0;

    // --- Asynchronous / streaming ---

    [[nodiscard]] virtual StreamRequestId AcquireAsync(
        const we::runtime::assetimporter::AssetGuid& guid,
        AssetLoadCallback onLoaded = {},
        AssetLoadProgressCallback onProgress = {},
        AssetStreamPriority priority = AssetStreamPriority::Normal) = 0;

    [[nodiscard]] virtual StreamRequestId AcquireAsync(
        const VirtualPath& path,
        AssetLoadCallback onLoaded = {},
        AssetLoadProgressCallback onProgress = {},
        AssetStreamPriority priority = AssetStreamPriority::Normal) = 0;

    [[nodiscard]] virtual StreamRequestId Stream(const StreamRequestDesc& desc) = 0;
    [[nodiscard]] virtual bool CancelStream(StreamRequestId requestId) = 0;

    [[nodiscard]] virtual bool Preload(const AssetBundleDesc& bundle) = 0;
    [[nodiscard]] virtual StreamRequestId PreloadAsync(const AssetBundleDesc& bundle) = 0;
    [[nodiscard]] virtual bool LoadBundle(const AssetBundleDesc& bundle) = 0;

    // --- Lifetime ---

    virtual void Release(AssetHandle handle) = 0;
    virtual void Release(const we::runtime::assetimporter::AssetGuid& guid) = 0;

    [[nodiscard]] virtual WeakAssetHandle GetWeakHandle(AssetHandle handle) const = 0;
    [[nodiscard]] virtual AssetHandle LockWeak(WeakAssetHandle weak) const = 0;

    [[nodiscard]] virtual std::shared_ptr<RuntimeAsset> TryGet(AssetHandle handle) const = 0;
    [[nodiscard]] virtual std::shared_ptr<RuntimeAsset> TryGet(
        const we::runtime::assetimporter::AssetGuid& guid) const = 0;

    [[nodiscard]] virtual AssetLoadState GetLoadState(AssetHandle handle) const = 0;
    [[nodiscard]] virtual AssetLoadState GetLoadState(
        const we::runtime::assetimporter::AssetGuid& guid) const = 0;

    /// Deterministically unload all zero-refcount residents (ignores sticky policy if force).
    virtual size_t UnloadUnused(bool force = false) = 0;

    // --- Memory / residency ---

    [[nodiscard]] virtual uint64_t GetResidentBytes() const = 0;
    [[nodiscard]] virtual uint64_t GetMemoryBudgetBytes() const = 0;
    virtual void SetMemoryBudgetBytes(uint64_t bytes) = 0;
    virtual void EnforceBudget() = 0;

    // --- Hot reload (Editor) ---

    [[nodiscard]] virtual bool HotReload(const we::runtime::assetimporter::AssetGuid& guid) = 0;
    [[nodiscard]] virtual bool HotReload(const VirtualPath& path) = 0;
    virtual void SetHotReloadEnabled(bool enabled) = 0;
    [[nodiscard]] virtual bool IsHotReloadEnabled() const = 0;

    // --- Extensibility ---

    [[nodiscard]] virtual AssetLoaderRegistry& GetLoaderRegistry() = 0;
    [[nodiscard]] virtual const AssetLoaderRegistry& GetLoaderRegistry() const = 0;
    [[nodiscard]] virtual PackageProviderRegistry& GetPackageProviderRegistry() = 0;
    [[nodiscard]] virtual const PackageProviderRegistry& GetPackageProviderRegistry() const = 0;

    // --- Diagnostics ---

    [[nodiscard]] virtual AssetManagerDiagnostics GetDiagnostics(
        bool includeResidencySnapshot = false) const = 0;
    [[nodiscard]] virtual AssetCacheStats GetCacheStats() const = 0;
    [[nodiscard]] virtual AssetStreamMetrics GetStreamMetrics() const = 0;

    virtual void WaitForIdle() = 0;
    virtual void Shutdown() = 0;
};

[[nodiscard]] ASSETRUNTIME_API std::unique_ptr<IAssetManager> CreateAssetManager(
    AssetRuntimeDependencies deps = {});

} // namespace we::runtime::assetruntime
