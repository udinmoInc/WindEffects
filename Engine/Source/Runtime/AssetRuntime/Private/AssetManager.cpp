#include "AssetRuntime/IAssetManager.h"

#include "BuiltinProviders.h"
#include "Core/Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>

namespace we::runtime::assetruntime {
namespace {

struct CatalogEntry {
    we::runtime::assetimporter::AssetGuid guid{};
    VirtualPath virtualPath;
    we::runtime::assetimporter::AssetKind kind = we::runtime::assetimporter::AssetKind::Unknown;
    std::string displayName;
    std::string contentType;
    std::vector<we::runtime::assetimporter::AssetGuid> dependencies;
    std::string mountId;
    int mountPriority = 0;
    uint64_t uncompressedSize = 0;
};

struct MountRecord {
    PackageMountInfo info{};
    std::shared_ptr<IPackageProvider> provider;
    void* state = nullptr;
};

struct HandleSlot {
    uint32_t generation = 1;
    uint32_t weakEpoch = 1;
    we::runtime::assetimporter::AssetGuid guid{};
    std::shared_ptr<RuntimeAsset> asset;
    bool occupied = false;
};

struct StreamJob {
    StreamRequestId id{};
    we::runtime::assetimporter::AssetGuid guid{};
    AssetStreamPriority priority = AssetStreamPriority::Normal;
    bool loadDependencies = true;
    bool cancelled = false;
    AssetLoadCallback onLoaded;
    AssetLoadProgressCallback onProgress;
};

class AssetManagerService final : public IAssetManager {
public:
    explicit AssetManagerService(AssetRuntimeDependencies deps)
        : m_Deps(std::move(deps))
        , m_Budget(m_Deps.memoryBudgetBytes)
        , m_HotReloadEnabled(m_Deps.enableHotReload) {
        RegisterBuiltinPackageProviders(m_Providers);
        RegisterBuiltinAssetLoaders(m_Loaders);
        // Slot 0 reserved as Invalid.
        m_Slots.resize(1);

        if (!m_Deps.contentRoot.empty()) {
            (void)MountContent(
                m_Deps.contentRoot, m_Deps.contentMountId, m_Deps.contentVirtualRoot, 0);
        }
        if (!m_Deps.packagePath.empty()) {
            PackageMountRequest req{};
            req.mountId = m_Deps.packageMountId;
            req.source = m_Deps.packagePath;
            req.virtualRoot = m_Deps.contentVirtualRoot;
            req.priority = 10;
            (void)MountPackage(req);
        }
    }

    ~AssetManagerService() override { Shutdown(); }

    [[nodiscard]] PackageMountResult MountPackage(const PackageMountRequest& request) override {
        PackageMountResult result{};
        result.mountId = request.mountId;

        if (request.mountId.empty()) {
            result.errorCode = "mount.id_empty";
            result.errorMessage = "Mount id must be non-empty.";
            return result;
        }
        if (request.source.empty()) {
            result.errorCode = "mount.source_empty";
            result.errorMessage = "Mount source path is empty.";
            return result;
        }

        auto provider = m_Providers.ResolveForPath(request.source);
        if (!provider) {
            result.errorCode = "mount.provider_missing";
            result.errorMessage = "No package provider can open the given source.";
            return result;
        }

        const auto opened = provider->Open(request.source, request.virtualRoot);
        if (!opened.success) {
            result.errorCode = opened.errorCode.empty() ? "mount.open_failed" : opened.errorCode;
            result.errorMessage = opened.errorMessage.empty()
                ? "Package provider failed to open source."
                : opened.errorMessage;
            return result;
        }

        if (request.expectedVersion != 0 && opened.packageVersion != 0
            && request.expectedVersion != opened.packageVersion) {
            result.errorCode = "mount.version_mismatch";
            result.errorMessage = "Package version does not match expectedVersion.";
            return result;
        }
        if (!request.expectedPackageId.empty() && !opened.packageId.empty()
            && request.expectedPackageId != opened.packageId) {
            result.errorCode = "mount.package_id_mismatch";
            result.errorMessage = "Package id does not match expectedPackageId.";
            return result;
        }

        void* state = provider->CreateState(request.source);
        if (!state) {
            result.errorCode = "mount.state_failed";
            result.errorMessage = "Provider failed to create package state.";
            return result;
        }

        {
            std::lock_guard lock(m_Mutex);
            if (m_Mounts.contains(request.mountId)) {
                provider->DestroyState(state);
                result.errorCode = "mount.already_mounted";
                result.errorMessage = "A package with this mountId is already mounted.";
                return result;
            }

            MountRecord mount{};
            mount.provider = provider;
            mount.state = state;
            mount.info.mountId = request.mountId;
            mount.info.providerId = std::string(provider->GetProviderId());
            mount.info.virtualRoot = request.virtualRoot;
            mount.info.sourcePath = request.source.generic_string();
            mount.info.packageVersion = opened.packageVersion;
            mount.info.priority = request.priority;
            mount.info.assetCount = opened.assets.size();
            mount.info.readOnly = request.readOnly;
            m_Mounts[request.mountId] = std::move(mount);

            for (const auto& asset : opened.assets) {
                CatalogEntry entry{};
                entry.guid = asset.guid;
                entry.virtualPath = asset.virtualPath;
                entry.kind = asset.kind;
                entry.displayName = asset.displayName;
                entry.contentType = asset.contentType;
                entry.dependencies = asset.dependencies;
                entry.mountId = request.mountId;
                entry.mountPriority = request.priority;
                entry.uncompressedSize = asset.uncompressedSize;
                InsertCatalogUnlocked(std::move(entry));
            }
        }

        result.success = true;
        result.packageVersion = opened.packageVersion;
        result.assetCount = opened.assets.size();
        Log("Mounted package '" + request.mountId + "' assets=" + std::to_string(result.assetCount));
        return result;
    }

    [[nodiscard]] bool UnmountPackage(std::string_view mountId) override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Mounts.find(std::string(mountId));
        if (it == m_Mounts.end()) {
            return false;
        }

        // Drop catalog entries belonging to this mount.
        for (auto catIt = m_Catalog.begin(); catIt != m_Catalog.end();) {
            if (catIt->second.mountId == mountId) {
                m_PathToGuid.erase(catIt->second.virtualPath);
                catIt = m_Catalog.erase(catIt);
            } else {
                ++catIt;
            }
        }

        // Unload residents from this mount with zero refs (force payload drop).
        for (auto& slot : m_Slots) {
            if (!slot.occupied || !slot.asset) {
                continue;
            }
            if (slot.asset->mountId == mountId) {
                if (slot.asset->refCount == 0) {
                    EvictSlotUnlocked(slot);
                } else {
                    // Keep handle valid but mark failed so gameplay can react.
                    slot.asset->state = AssetLoadState::Failed;
                }
            }
        }

        if (it->second.provider && it->second.state) {
            it->second.provider->DestroyState(it->second.state);
        }
        m_Mounts.erase(it);
        return true;
    }

    [[nodiscard]] std::vector<PackageMountInfo> ListMounts() const override {
        std::lock_guard lock(m_Mutex);
        std::vector<PackageMountInfo> out;
        out.reserve(m_Mounts.size());
        for (const auto& [_, mount] : m_Mounts) {
            out.push_back(mount.info);
        }
        std::sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
            return a.priority > b.priority;
        });
        return out;
    }

    [[nodiscard]] PackageMountResult MountContent(
        const std::filesystem::path& contentRoot,
        std::string_view mountId,
        std::string_view virtualRoot,
        int priority) override {
        PackageMountRequest req{};
        req.mountId = std::string(mountId);
        req.source = contentRoot;
        req.virtualRoot = std::string(virtualRoot);
        req.priority = priority;
        return MountPackage(req);
    }

    [[nodiscard]] std::optional<we::runtime::assetimporter::AssetGuid> ResolveGuid(
        const VirtualPath& path) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_PathToGuid.find(path);
        if (it == m_PathToGuid.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    [[nodiscard]] std::optional<VirtualPath> ResolvePath(
        const we::runtime::assetimporter::AssetGuid& guid) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Catalog.find(guid);
        if (it == m_Catalog.end()) {
            return std::nullopt;
        }
        return it->second.virtualPath;
    }

    [[nodiscard]] bool Contains(const we::runtime::assetimporter::AssetGuid& guid) const override {
        std::lock_guard lock(m_Mutex);
        return m_Catalog.contains(guid);
    }

    [[nodiscard]] bool Contains(const VirtualPath& path) const override {
        std::lock_guard lock(m_Mutex);
        return m_PathToGuid.contains(path);
    }

    [[nodiscard]] AssetHandle Acquire(const we::runtime::assetimporter::AssetGuid& guid) override {
        return AcquireInternal(guid, true);
    }

    [[nodiscard]] AssetHandle Acquire(const VirtualPath& path) override {
        auto guid = ResolveGuid(path);
        if (!guid) {
            ++m_Stats.failedLoads;
            return AssetHandle::Invalid();
        }
        return AcquireInternal(*guid, true);
    }

    [[nodiscard]] AssetHandle AcquireLazy(const we::runtime::assetimporter::AssetGuid& guid) override {
        return AcquireInternal(guid, false);
    }

    [[nodiscard]] StreamRequestId AcquireAsync(
        const we::runtime::assetimporter::AssetGuid& guid,
        AssetLoadCallback onLoaded,
        AssetLoadProgressCallback onProgress,
        AssetStreamPriority priority) override {
        StreamRequestDesc desc{};
        desc.guid = guid;
        desc.priority = priority;
        desc.loadDependencies = true;
        desc.onLoaded = std::move(onLoaded);
        desc.onProgress = std::move(onProgress);
        return Stream(desc);
    }

    [[nodiscard]] StreamRequestId AcquireAsync(
        const VirtualPath& path,
        AssetLoadCallback onLoaded,
        AssetLoadProgressCallback onProgress,
        AssetStreamPriority priority) override {
        StreamRequestDesc desc{};
        desc.path = path;
        desc.priority = priority;
        desc.loadDependencies = true;
        desc.onLoaded = std::move(onLoaded);
        desc.onProgress = std::move(onProgress);
        return Stream(desc);
    }

    [[nodiscard]] StreamRequestId Stream(const StreamRequestDesc& desc) override {
        we::runtime::assetimporter::AssetGuid guid = desc.guid;
        if (guid.IsNil() && !desc.path.Empty()) {
            auto resolved = ResolveGuid(desc.path);
            if (!resolved) {
                ++m_Stats.failedLoads;
                if (desc.onLoaded) {
                    desc.onLoaded(AssetHandle::Invalid(), nullptr);
                }
                return StreamRequestId::Invalid();
            }
            guid = *resolved;
        }
        if (guid.IsNil()) {
            ++m_Stats.failedLoads;
            return StreamRequestId::Invalid();
        }

        // Fast path: already resident.
        {
            std::lock_guard lock(m_Mutex);
            if (auto existing = FindResidentUnlocked(guid)) {
                ++existing->refCount;
                TouchLruUnlocked(guid);
                ++m_Stats.cacheHits;
                ++m_Stats.asyncLoads;
                const AssetHandle handle = existing->handle;
                if (desc.onLoaded) {
                    desc.onLoaded(handle, existing);
                }
                return StreamRequestId::Invalid(); // completed synchronously
            }
        }

        EnsureWorkers();
        StreamJob job{};
        job.id.value = m_NextRequestId.fetch_add(1);
        job.guid = guid;
        job.priority = desc.priority;
        job.loadDependencies = desc.loadDependencies;
        job.onLoaded = desc.onLoaded;
        job.onProgress = desc.onProgress;

        {
            std::lock_guard lock(m_Mutex);
            m_Jobs[job.id.value] = job;
            InsertJobQueueUnlocked(job);
            m_StreamMetrics.requestsSubmitted++;
            m_StreamMetrics.pendingJobs = static_cast<uint32_t>(m_JobQueue.size());
            m_StreamMetrics.peakQueueDepth = std::max(
                m_StreamMetrics.peakQueueDepth, static_cast<uint64_t>(m_JobQueue.size()));
        }
        m_Cv.notify_one();
        ++m_Stats.asyncLoads;
        return job.id;
    }

    [[nodiscard]] bool CancelStream(StreamRequestId requestId) override {
        if (!requestId.IsValid()) {
            return false;
        }
        std::lock_guard lock(m_Mutex);
        const auto it = m_Jobs.find(requestId.value);
        if (it == m_Jobs.end()) {
            return false;
        }
        it->second.cancelled = true;
        for (auto& queued : m_JobQueue) {
            if (queued.id.value == requestId.value) {
                queued.cancelled = true;
            }
        }
        m_StreamMetrics.requestsCancelled++;
        return true;
    }

    [[nodiscard]] bool Preload(const AssetBundleDesc& bundle) override {
        return LoadBundle(bundle);
    }

    [[nodiscard]] StreamRequestId PreloadAsync(const AssetBundleDesc& bundle) override {
        StreamRequestId last{};
        for (const auto& guid : bundle.assets) {
            last = AcquireAsync(guid, {}, {}, bundle.priority);
        }
        for (const auto& path : bundle.paths) {
            last = AcquireAsync(path, {}, {}, bundle.priority);
        }
        return last;
    }

    [[nodiscard]] bool LoadBundle(const AssetBundleDesc& bundle) override {
        bool ok = true;
        for (const auto& guid : bundle.assets) {
            if (AcquireInternal(guid, bundle.loadDependencies).IsValid() == false) {
                ok = false;
            }
        }
        for (const auto& path : bundle.paths) {
            auto guid = ResolveGuid(path);
            if (!guid || !AcquireInternal(*guid, bundle.loadDependencies).IsValid()) {
                ok = false;
            }
        }
        return ok;
    }

    void Release(AssetHandle handle) override {
        std::lock_guard lock(m_Mutex);
        auto* slot = GetSlotUnlocked(handle);
        if (!slot || !slot->asset) {
            return;
        }
        if (slot->asset->refCount > 0) {
            --slot->asset->refCount;
        }
    }

    void Release(const we::runtime::assetimporter::AssetGuid& guid) override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_GuidToHandle.find(guid);
        if (it == m_GuidToHandle.end()) {
            return;
        }
        auto* slot = GetSlotUnlocked(it->second);
        if (!slot || !slot->asset) {
            return;
        }
        if (slot->asset->refCount > 0) {
            --slot->asset->refCount;
        }
    }

    [[nodiscard]] WeakAssetHandle GetWeakHandle(AssetHandle handle) const override {
        std::lock_guard lock(m_Mutex);
        const auto* slot = GetSlotUnlocked(handle);
        if (!slot || !slot->occupied) {
            return WeakAssetHandle::Invalid();
        }
        WeakAssetHandle weak{};
        weak.handle = handle;
        weak.weakEpoch = slot->weakEpoch;
        return weak;
    }

    [[nodiscard]] AssetHandle LockWeak(WeakAssetHandle weak) const override {
        std::lock_guard lock(m_Mutex);
        const auto* slot = GetSlotUnlocked(weak.handle);
        if (!slot || !slot->occupied || slot->weakEpoch != weak.weakEpoch) {
            return AssetHandle::Invalid();
        }
        if (!slot->asset || slot->asset->state != AssetLoadState::Loaded) {
            return AssetHandle::Invalid();
        }
        return weak.handle;
    }

    [[nodiscard]] std::shared_ptr<RuntimeAsset> TryGet(AssetHandle handle) const override {
        std::lock_guard lock(m_Mutex);
        const auto* slot = GetSlotUnlocked(handle);
        if (!slot || !slot->asset) {
            return nullptr;
        }
        return slot->asset;
    }

    [[nodiscard]] std::shared_ptr<RuntimeAsset> TryGet(
        const we::runtime::assetimporter::AssetGuid& guid) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_GuidToHandle.find(guid);
        if (it == m_GuidToHandle.end()) {
            return nullptr;
        }
        const auto* slot = GetSlotUnlocked(it->second);
        return slot ? slot->asset : nullptr;
    }

    [[nodiscard]] AssetLoadState GetLoadState(AssetHandle handle) const override {
        auto asset = TryGet(handle);
        return asset ? asset->state : AssetLoadState::Unloaded;
    }

    [[nodiscard]] AssetLoadState GetLoadState(
        const we::runtime::assetimporter::AssetGuid& guid) const override {
        auto asset = TryGet(guid);
        return asset ? asset->state : AssetLoadState::Unloaded;
    }

    size_t UnloadUnused(bool force) override {
        std::lock_guard lock(m_Mutex);
        size_t count = 0;
        for (auto it = m_Lru.begin(); it != m_Lru.end();) {
            const auto handleIt = m_GuidToHandle.find(*it);
            if (handleIt == m_GuidToHandle.end()) {
                it = m_Lru.erase(it);
                continue;
            }
            auto* slot = GetSlotUnlocked(handleIt->second);
            if (!slot || !slot->asset) {
                it = m_Lru.erase(it);
                continue;
            }
            if (slot->asset->refCount == 0 || force) {
                if (force) {
                    slot->asset->refCount = 0;
                }
                EvictSlotUnlocked(*slot);
                it = m_Lru.erase(it);
                ++count;
                ++m_Stats.evictions;
            } else {
                ++it;
            }
        }
        return count;
    }

    [[nodiscard]] uint64_t GetResidentBytes() const override {
        std::lock_guard lock(m_Mutex);
        return m_ResidentBytes;
    }

    [[nodiscard]] uint64_t GetMemoryBudgetBytes() const override {
        return m_Budget.load();
    }

    void SetMemoryBudgetBytes(uint64_t bytes) override {
        m_Budget.store(bytes);
        EnforceBudget();
    }

    void EnforceBudget() override {
        std::lock_guard lock(m_Mutex);
        EnforceBudgetUnlocked();
    }

    [[nodiscard]] bool HotReload(const we::runtime::assetimporter::AssetGuid& guid) override {
        if (!m_HotReloadEnabled.load()) {
            return false;
        }
        {
            std::lock_guard lock(m_Mutex);
            const auto it = m_GuidToHandle.find(guid);
            if (it != m_GuidToHandle.end()) {
                auto* slot = GetSlotUnlocked(it->second);
                if (slot && slot->asset) {
                    if (m_ResidentBytes >= slot->asset->residentBytes) {
                        m_ResidentBytes -= slot->asset->residentBytes;
                    }
                    slot->asset->payload.clear();
                    slot->asset->decodedObject.reset();
                    slot->asset->residentBytes = 0;
                    slot->asset->state = AssetLoadState::Unloaded;
                }
            }
        }

        auto asset = LoadInternal(guid);
        if (!asset) {
            return false;
        }
        ++m_Stats.hotReloads;
        if (m_Deps.onHotReload) {
            m_Deps.onHotReload(asset->handle, asset);
        }
        return true;
    }

    [[nodiscard]] bool HotReload(const VirtualPath& path) override {
        auto guid = ResolveGuid(path);
        return guid ? HotReload(*guid) : false;
    }

    void SetHotReloadEnabled(bool enabled) override { m_HotReloadEnabled.store(enabled); }

    [[nodiscard]] bool IsHotReloadEnabled() const override { return m_HotReloadEnabled.load(); }

    [[nodiscard]] AssetLoaderRegistry& GetLoaderRegistry() override { return m_Loaders; }
    [[nodiscard]] const AssetLoaderRegistry& GetLoaderRegistry() const override { return m_Loaders; }
    [[nodiscard]] PackageProviderRegistry& GetPackageProviderRegistry() override {
        return m_Providers;
    }
    [[nodiscard]] const PackageProviderRegistry& GetPackageProviderRegistry() const override {
        return m_Providers;
    }

    [[nodiscard]] AssetManagerDiagnostics GetDiagnostics(bool includeResidencySnapshot) const override {
        AssetManagerDiagnostics diag{};
        diag.cache = GetCacheStats();
        diag.streaming = GetStreamMetrics();
        diag.mounts = ListMounts();
        if (includeResidencySnapshot) {
            std::lock_guard lock(m_Mutex);
            for (const auto& slot : m_Slots) {
                if (!slot.occupied || !slot.asset) {
                    continue;
                }
                AssetResidencyInfo info{};
                info.guid = slot.asset->guid;
                info.virtualPath = slot.asset->virtualPath.Get();
                info.state = slot.asset->state;
                info.refCount = slot.asset->refCount;
                info.residentBytes = slot.asset->residentBytes;
                info.lastLoadMicroseconds = slot.asset->lastLoadMicroseconds;
                info.mountId = slot.asset->mountId;
                diag.residency.push_back(std::move(info));
            }
        }
        return diag;
    }

    [[nodiscard]] AssetCacheStats GetCacheStats() const override {
        std::lock_guard lock(m_Mutex);
        AssetCacheStats stats = m_Stats;
        stats.catalogEntries = m_Catalog.size();
        stats.residentAssets = m_GuidToHandle.size();
        stats.residentBytes = m_ResidentBytes;
        stats.memoryBudgetBytes = m_Budget.load();
        if (stats.syncLoads + stats.asyncLoads > 0 && m_TotalLoadMicroseconds > 0) {
            stats.averageLoadMilliseconds =
                (static_cast<double>(m_TotalLoadMicroseconds) / 1000.0)
                / static_cast<double>(stats.syncLoads + stats.asyncLoads);
        }
        stats.peakLoadMilliseconds = static_cast<double>(m_PeakLoadMicroseconds) / 1000.0;
        return stats;
    }

    [[nodiscard]] AssetStreamMetrics GetStreamMetrics() const override {
        std::lock_guard lock(m_Mutex);
        AssetStreamMetrics metrics = m_StreamMetrics;
        metrics.activeWorkers = m_ActiveWorkers;
        metrics.pendingJobs = static_cast<uint32_t>(m_JobQueue.size());
        return metrics;
    }

    void WaitForIdle() override {
        for (;;) {
            {
                std::lock_guard lock(m_Mutex);
                if (m_JobQueue.empty() && m_ActiveWorkers == 0) {
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
    }

    void Shutdown() override {
        {
            std::lock_guard lock(m_Mutex);
            m_Stopping = true;
            for (auto& job : m_JobQueue) {
                job.cancelled = true;
            }
        }
        m_Cv.notify_all();
        for (auto& w : m_Workers) {
            if (w.joinable()) {
                w.join();
            }
        }
        m_Workers.clear();

        std::lock_guard lock(m_Mutex);
        for (auto& [_, mount] : m_Mounts) {
            if (mount.provider && mount.state) {
                mount.provider->DestroyState(mount.state);
                mount.state = nullptr;
            }
        }
        m_Mounts.clear();
        m_Catalog.clear();
        m_PathToGuid.clear();
        m_GuidToHandle.clear();
        m_Lru.clear();
        m_Slots.clear();
        m_Slots.resize(1);
        m_FreeSlots.clear();
        m_ResidentBytes = 0;
        m_Jobs.clear();
        m_JobQueue.clear();
    }

private:
    void Log(std::string_view message) const {
        if (m_Deps.onLog) {
            m_Deps.onLog(message);
        }
    }

    void InsertCatalogUnlocked(CatalogEntry entry) {
        const auto existing = m_Catalog.find(entry.guid);
        if (existing != m_Catalog.end()) {
            if (entry.mountPriority < existing->second.mountPriority) {
                return; // lower priority loses
            }
            m_PathToGuid.erase(existing->second.virtualPath);
        }
        if (!entry.virtualPath.Empty()) {
            m_PathToGuid[entry.virtualPath] = entry.guid;
        }
        m_Catalog[entry.guid] = std::move(entry);
    }

    AssetHandle AcquireInternal(
        const we::runtime::assetimporter::AssetGuid& guid,
        bool loadDependencies) {
        if (guid.IsNil()) {
            return AssetHandle::Invalid();
        }

        if (loadDependencies) {
            if (!LoadDependenciesRecursive(guid, {})) {
                ++m_Stats.failedLoads;
                return AssetHandle::Invalid();
            }
        }

        auto asset = LoadInternal(guid);
        if (!asset) {
            ++m_Stats.failedLoads;
            return AssetHandle::Invalid();
        }

        {
            std::lock_guard lock(m_Mutex);
            ++asset->refCount;
            TouchLruUnlocked(guid);
            ++m_Stats.syncLoads;
        }
        EnforceBudget();
        return asset->handle;
    }

    bool LoadDependenciesRecursive(
        const we::runtime::assetimporter::AssetGuid& guid,
        std::unordered_set<
            we::runtime::assetimporter::AssetGuid,
            we::runtime::assetimporter::AssetGuidHash> visiting) {
        if (visiting.contains(guid)) {
            return false; // cycle
        }
        CatalogEntry cat;
        {
            std::lock_guard lock(m_Mutex);
            const auto it = m_Catalog.find(guid);
            if (it == m_Catalog.end()) {
                return false;
            }
            cat = it->second;
        }
        visiting.insert(guid);
        for (const auto& dep : cat.dependencies) {
            if (!LoadDependenciesRecursive(dep, visiting)) {
                return false;
            }
            if (!AcquireInternal(dep, false).IsValid()) {
                return false;
            }
        }
        return true;
    }

    std::shared_ptr<RuntimeAsset> LoadInternal(const we::runtime::assetimporter::AssetGuid& guid) {
        {
            std::lock_guard lock(m_Mutex);
            if (auto existing = FindResidentUnlocked(guid)) {
                if (existing->state == AssetLoadState::Loaded && !existing->payload.empty()) {
                    ++m_Stats.cacheHits;
                    return existing;
                }
            }
            ++m_Stats.cacheMisses;
        }

        CatalogEntry cat;
        MountRecord mountCopy;
        {
            std::lock_guard lock(m_Mutex);
            const auto it = m_Catalog.find(guid);
            if (it == m_Catalog.end()) {
                return nullptr;
            }
            cat = it->second;
            const auto mountIt = m_Mounts.find(cat.mountId);
            if (mountIt == m_Mounts.end()) {
                return nullptr;
            }
            mountCopy.provider = mountIt->second.provider;
            mountCopy.state = mountIt->second.state;
            mountCopy.info = mountIt->second.info;
        }

        const auto start = std::chrono::steady_clock::now();
        auto read = mountCopy.provider->ReadAsset(mountCopy.state, guid);
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - start);

        if (!read.success) {
            return nullptr;
        }

        std::shared_ptr<RuntimeAsset> asset;
        AssetHandle handle{};
        {
            std::lock_guard lock(m_Mutex);
            // Reuse slot if present.
            if (auto existing = FindResidentUnlocked(guid)) {
                asset = existing;
                handle = existing->handle;
                if (m_ResidentBytes >= asset->residentBytes) {
                    m_ResidentBytes -= asset->residentBytes;
                }
            } else {
                handle = AllocateHandleUnlocked(guid);
                asset = std::make_shared<RuntimeAsset>();
                auto* slot = GetSlotUnlocked(handle);
                slot->asset = asset;
                slot->guid = guid;
                m_GuidToHandle[guid] = handle;
            }

            asset->guid = guid;
            asset->virtualPath = cat.virtualPath;
            asset->kind = read.kind != we::runtime::assetimporter::AssetKind::Unknown
                ? read.kind
                : cat.kind;
            asset->displayName = !read.displayName.empty() ? read.displayName : cat.displayName;
            asset->mountId = cat.mountId;
            asset->contentType = !read.contentType.empty() ? read.contentType : cat.contentType;
            asset->payload = std::move(read.payload);
            asset->dependencies = !read.dependencies.empty() ? read.dependencies : cat.dependencies;
            asset->residentBytes = asset->payload.size();
            asset->lastLoadMicroseconds = static_cast<uint64_t>(elapsed.count());
            asset->state = AssetLoadState::Loaded;
            asset->handle = handle;
            m_ResidentBytes += asset->residentBytes;
            TouchLruUnlocked(guid);
        }

        m_TotalLoadMicroseconds += static_cast<uint64_t>(elapsed.count());
        m_PeakLoadMicroseconds = std::max(
            m_PeakLoadMicroseconds, static_cast<uint64_t>(elapsed.count()));

        if (m_Deps.decodeOnLoad) {
            auto loader = m_Loaders.Resolve(asset->kind, asset->contentType);
            if (loader) {
                const auto decoded = loader->Decode(*asset, asset->payload);
                if (decoded.success) {
                    asset->decodedObject = decoded.object;
                    asset->decodedTypeId = decoded.typeId;
                }
            }
        }

        return asset;
    }

    std::shared_ptr<RuntimeAsset> FindResidentUnlocked(
        const we::runtime::assetimporter::AssetGuid& guid) const {
        const auto it = m_GuidToHandle.find(guid);
        if (it == m_GuidToHandle.end()) {
            return nullptr;
        }
        const auto* slot = GetSlotUnlocked(it->second);
        return slot ? slot->asset : nullptr;
    }

    AssetHandle AllocateHandleUnlocked(const we::runtime::assetimporter::AssetGuid& guid) {
        uint32_t index = 0;
        if (!m_FreeSlots.empty()) {
            index = m_FreeSlots.back();
            m_FreeSlots.pop_back();
        } else {
            index = static_cast<uint32_t>(m_Slots.size());
            m_Slots.emplace_back();
        }
        auto& slot = m_Slots[index];
        slot.occupied = true;
        slot.guid = guid;
        if (slot.generation == 0) {
            slot.generation = 1;
        }
        AssetHandle handle{};
        handle.index = index;
        handle.generation = slot.generation;
        return handle;
    }

    HandleSlot* GetSlotUnlocked(AssetHandle handle) {
        if (!handle.IsValid() || handle.index >= m_Slots.size()) {
            return nullptr;
        }
        auto& slot = m_Slots[handle.index];
        if (!slot.occupied || slot.generation != handle.generation) {
            return nullptr;
        }
        return &slot;
    }

    const HandleSlot* GetSlotUnlocked(AssetHandle handle) const {
        return const_cast<AssetManagerService*>(this)->GetSlotUnlocked(handle);
    }

    void EvictSlotUnlocked(HandleSlot& slot) {
        if (!slot.asset) {
            return;
        }
        if (m_ResidentBytes >= slot.asset->residentBytes) {
            m_ResidentBytes -= slot.asset->residentBytes;
        }
        const auto guid = slot.guid;
        slot.asset->payload.clear();
        slot.asset->decodedObject.reset();
        slot.asset->residentBytes = 0;
        slot.asset->state = AssetLoadState::Evicted;
        slot.asset->refCount = 0;
        m_GuidToHandle.erase(guid);
        slot.asset.reset();
        slot.occupied = false;
        ++slot.generation;
        if (slot.generation == 0) {
            slot.generation = 1;
        }
        ++slot.weakEpoch;
        if (slot.weakEpoch == 0) {
            slot.weakEpoch = 1;
        }
        m_FreeSlots.push_back(
            static_cast<uint32_t>(&slot - m_Slots.data()));
    }

    void TouchLruUnlocked(const we::runtime::assetimporter::AssetGuid& guid) {
        m_Lru.remove(guid);
        m_Lru.push_back(guid);
    }

    void EnforceBudgetUnlocked() {
        const uint64_t budget = m_Budget.load();
        while (m_ResidentBytes > budget && !m_Lru.empty()) {
            bool evicted = false;
            for (auto it = m_Lru.begin(); it != m_Lru.end();) {
                const auto handleIt = m_GuidToHandle.find(*it);
                if (handleIt == m_GuidToHandle.end()) {
                    it = m_Lru.erase(it);
                    continue;
                }
                auto* slot = GetSlotUnlocked(handleIt->second);
                if (!slot || !slot->asset) {
                    it = m_Lru.erase(it);
                    continue;
                }
                if (slot->asset->refCount == 0) {
                    EvictSlotUnlocked(*slot);
                    it = m_Lru.erase(it);
                    ++m_Stats.evictions;
                    evicted = true;
                    break;
                }
                ++it;
            }
            if (!evicted) {
                break;
            }
        }
    }

    void InsertJobQueueUnlocked(const StreamJob& job) {
        auto insertAt = m_JobQueue.begin();
        while (insertAt != m_JobQueue.end()
            && static_cast<int>(insertAt->priority) >= static_cast<int>(job.priority)) {
            ++insertAt;
        }
        m_JobQueue.insert(insertAt, job);
    }

    void EnsureWorkers() {
        std::lock_guard lock(m_Mutex);
        if (!m_Workers.empty()) {
            return;
        }
        unsigned count = m_Deps.workerThreads;
        if (count == 0) {
            count = std::min(4u, std::max(1u, std::thread::hardware_concurrency()));
        }
        m_Stopping = false;
        for (unsigned i = 0; i < count; ++i) {
            m_Workers.emplace_back([this] { WorkerLoop(); });
        }
    }

    void WorkerLoop() {
        for (;;) {
            StreamJob job;
            {
                std::unique_lock lock(m_Mutex);
                m_Cv.wait(lock, [&] { return m_Stopping || !m_JobQueue.empty(); });
                if (m_Stopping && m_JobQueue.empty()) {
                    return;
                }
                job = std::move(m_JobQueue.front());
                m_JobQueue.pop_front();
                ++m_ActiveWorkers;
                m_StreamMetrics.pendingJobs = static_cast<uint32_t>(m_JobQueue.size());
            }

            if (job.cancelled) {
                {
                    std::lock_guard lock(m_Mutex);
                    m_Jobs.erase(job.id.value);
                    --m_ActiveWorkers;
                }
                if (job.onLoaded) {
                    job.onLoaded(AssetHandle::Invalid(), nullptr);
                }
                continue;
            }

            if (job.onProgress) {
                AssetLoadProgress p{};
                p.guid = job.guid;
                p.requestId = job.id;
                p.state = AssetLoadState::Loading;
                p.fraction = 0.1f;
                p.stage = "loading";
                job.onProgress(p);
            }

            AssetHandle handle = AssetHandle::Invalid();
            std::shared_ptr<RuntimeAsset> asset;
            if (job.loadDependencies) {
                handle = AcquireInternal(job.guid, true);
            } else {
                handle = AcquireInternal(job.guid, false);
            }
            asset = TryGet(handle);

            if (asset) {
                std::lock_guard lock(m_Mutex);
                m_StreamMetrics.bytesStreamed += asset->residentBytes;
                m_StreamMetrics.requestsCompleted++;
            } else {
                std::lock_guard lock(m_Mutex);
                m_StreamMetrics.requestsFailed++;
            }

            if (job.onProgress) {
                AssetLoadProgress p{};
                p.guid = job.guid;
                p.handle = handle;
                p.requestId = job.id;
                p.state = asset ? AssetLoadState::Loaded : AssetLoadState::Failed;
                p.fraction = 1.0f;
                p.stage = asset ? "loaded" : "failed";
                job.onProgress(p);
            }
            if (job.onLoaded) {
                job.onLoaded(handle, asset);
            }

            {
                std::lock_guard lock(m_Mutex);
                m_Jobs.erase(job.id.value);
                --m_ActiveWorkers;
            }
        }
    }

    AssetRuntimeDependencies m_Deps{};
    AssetLoaderRegistry m_Loaders;
    PackageProviderRegistry m_Providers;

    mutable std::mutex m_Mutex;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        CatalogEntry,
        we::runtime::assetimporter::AssetGuidHash>
        m_Catalog;
    std::unordered_map<VirtualPath, we::runtime::assetimporter::AssetGuid, VirtualPathHash>
        m_PathToGuid;
    std::unordered_map<std::string, MountRecord> m_Mounts;

    std::vector<HandleSlot> m_Slots;
    std::vector<uint32_t> m_FreeSlots;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        AssetHandle,
        we::runtime::assetimporter::AssetGuidHash>
        m_GuidToHandle;
    std::list<we::runtime::assetimporter::AssetGuid> m_Lru;
    uint64_t m_ResidentBytes = 0;
    std::atomic<uint64_t> m_Budget{ 512ull * 1024ull * 1024ull };
    std::atomic<bool> m_HotReloadEnabled{ false };

    AssetCacheStats m_Stats{};
    AssetStreamMetrics m_StreamMetrics{};
    uint64_t m_TotalLoadMicroseconds = 0;
    uint64_t m_PeakLoadMicroseconds = 0;

    std::condition_variable m_Cv;
    std::deque<StreamJob> m_JobQueue;
    std::unordered_map<uint64_t, StreamJob> m_Jobs;
    std::vector<std::thread> m_Workers;
    unsigned m_ActiveWorkers = 0;
    bool m_Stopping = false;
    std::atomic<uint64_t> m_NextRequestId{ 1 };
};

} // namespace

std::unique_ptr<IAssetManager> CreateAssetManager(AssetRuntimeDependencies deps) {
    return std::make_unique<AssetManagerService>(std::move(deps));
}

} // namespace we::runtime::assetruntime
