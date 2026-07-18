#include "AssetRuntime/IAssetManager.h"

#include "AssetImporter/AssetMetadata.h"
#include "AssetImporter/AssetPackage.h"
#include "Core/Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <list>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace we::runtime::assetruntime {
namespace {

struct CatalogEntry {
    we::runtime::assetimporter::AssetGuid guid{};
    std::filesystem::path loosePath;
    bool inPackage = false;
    uint32_t packageOffset = 0;
    uint32_t packageSize = 0;
    std::string contentType;
    we::runtime::assetimporter::AssetKind kind = we::runtime::assetimporter::AssetKind::Unknown;
    std::string displayName;
    std::vector<we::runtime::assetimporter::AssetGuid> dependencies;
};

struct AsyncLoadJob {
    we::runtime::assetimporter::AssetGuid guid{};
    AssetLoadCallback onLoaded;
    AssetLoadProgressCallback onProgress;
};

class AssetManagerService final : public IAssetManager {
public:
    explicit AssetManagerService(AssetRuntimeDependencies deps)
        : m_Deps(std::move(deps))
        , m_Budget(m_Deps.memoryBudgetBytes) {
        if (!m_Deps.contentRoot.empty()) {
            (void)MountContent(m_Deps.contentRoot);
        }
        if (!m_Deps.packagePath.empty()) {
            (void)MountPackage(m_Deps.packagePath);
        }
    }

    ~AssetManagerService() override { Shutdown(); }

    [[nodiscard]] bool MountContent(const std::filesystem::path& contentRoot) override {
        if (!std::filesystem::exists(contentRoot)) {
            return false;
        }
        m_ContentRoot = contentRoot;
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(contentRoot)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto& path = entry.path();
            const auto ext = path.extension().string();
            if (ext != ".weasset" && ext != ".wefont" && ext != ".weiconatlas") {
                continue;
            }
            CatalogEntry cat{};
            cat.loosePath = path;
            const auto metaPath = path.parent_path() / (path.stem().string() + ".wemeta");
            if (auto meta = we::runtime::assetimporter::LoadMetadataJson(metaPath)) {
                cat.guid = meta->guid;
                cat.kind = meta->kind;
                cat.displayName = meta->displayName;
                cat.dependencies = meta->dependencies;
            } else if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(path)) {
                cat.guid = pkg->metadata.guid;
                cat.kind = pkg->metadata.kind;
                cat.displayName = pkg->metadata.displayName;
                cat.dependencies = pkg->metadata.dependencies;
            }
            if (cat.guid.IsNil()) {
                continue;
            }
            std::lock_guard lock(m_Mutex);
            m_Catalog[cat.guid] = std::move(cat);
        }
        return true;
    }

    [[nodiscard]] bool MountPackage(const std::filesystem::path& wepakPath) override {
        auto archive = we::runtime::assetcooker::ReadWepak(wepakPath);
        if (!archive) {
            return false;
        }
        m_PackagePath = wepakPath;
        m_Package = std::move(*archive);
        std::lock_guard lock(m_Mutex);
        for (const auto& rec : m_Package.toc) {
            if (rec.guid.IsNil()) {
                continue;
            }
            CatalogEntry cat{};
            cat.guid = rec.guid;
            cat.inPackage = true;
            cat.packageOffset = rec.offset;
            cat.packageSize = static_cast<uint32_t>(rec.storedSize);
            cat.contentType = rec.contentType;
            cat.displayName = rec.relativePath;
            cat.loosePath = rec.relativePath;
            m_Catalog[cat.guid] = std::move(cat);
        }
        return true;
    }

    [[nodiscard]] std::shared_ptr<RuntimeAsset> Acquire(
        const we::runtime::assetimporter::AssetGuid& guid) override {
        auto asset = LoadInternal(guid);
        if (!asset) {
            return nullptr;
        }
        {
            std::lock_guard lock(m_Mutex);
            ++asset->refCount;
            TouchLru(guid);
        }
        EnforceBudget();
        return asset;
    }

    [[nodiscard]] we::runtime::assetimporter::AssetGuid AcquireAsync(
        const we::runtime::assetimporter::AssetGuid& guid,
        AssetLoadCallback onLoaded,
        AssetLoadProgressCallback onProgress) override {
        EnsureWorkers();
        {
            std::lock_guard lock(m_Mutex);
            auto existing = m_Resident.find(guid);
            if (existing != m_Resident.end() && existing->second
                && existing->second->state == AssetLoadState::Loaded) {
                ++existing->second->refCount;
                TouchLru(guid);
                if (onLoaded) {
                    onLoaded(existing->second);
                }
                return guid;
            }
        }

        AsyncLoadJob job{};
        job.guid = guid;
        job.onLoaded = std::move(onLoaded);
        job.onProgress = std::move(onProgress);
        {
            std::lock_guard lock(m_Mutex);
            m_LoadQueue.push_back(std::move(job));
        }
        m_Cv.notify_one();
        return guid;
    }

    void Release(const we::runtime::assetimporter::AssetGuid& guid) override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Resident.find(guid);
        if (it == m_Resident.end() || !it->second) {
            return;
        }
        if (it->second->refCount > 0) {
            --it->second->refCount;
        }
    }

    [[nodiscard]] std::shared_ptr<RuntimeAsset> TryGet(
        const we::runtime::assetimporter::AssetGuid& guid) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Resident.find(guid);
        if (it == m_Resident.end()) {
            return nullptr;
        }
        return it->second;
    }

    [[nodiscard]] bool LoadBundle(const AssetBundleDesc& bundle) override {
        bool ok = true;
        for (const auto& guid : bundle.assets) {
            if (bundle.loadDependencies) {
                if (!LoadWithDependencies(guid)) {
                    ok = false;
                }
            } else if (!Acquire(guid)) {
                ok = false;
            }
        }
        return ok;
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

    void WaitForIdle() override {
        for (;;) {
            {
                std::lock_guard lock(m_Mutex);
                if (m_LoadQueue.empty() && m_ActiveWorkers == 0) {
                    return;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    void Shutdown() override {
        {
            std::lock_guard lock(m_Mutex);
            m_Stopping = true;
        }
        m_Cv.notify_all();
        for (auto& w : m_Workers) {
            if (w.joinable()) {
                w.join();
            }
        }
        m_Workers.clear();
        std::lock_guard lock(m_Mutex);
        m_Resident.clear();
        m_Lru.clear();
        m_ResidentBytes = 0;
    }

private:
    void EnforceBudgetUnlocked() {
        const uint64_t budget = m_Budget.load();
        while (m_ResidentBytes > budget && !m_Lru.empty()) {
            bool evicted = false;
            for (auto it = m_Lru.begin(); it != m_Lru.end();) {
                const auto res = m_Resident.find(*it);
                if (res == m_Resident.end() || !res->second) {
                    it = m_Lru.erase(it);
                    continue;
                }
                if (res->second->refCount == 0) {
                    m_ResidentBytes -= res->second->residentBytes;
                    res->second->payload.clear();
                    res->second->residentBytes = 0;
                    res->second->state = AssetLoadState::Evicted;
                    m_Resident.erase(res);
                    it = m_Lru.erase(it);
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

    bool LoadWithDependencies(const we::runtime::assetimporter::AssetGuid& guid) {
        CatalogEntry cat;
        {
            std::lock_guard lock(m_Mutex);
            const auto it = m_Catalog.find(guid);
            if (it == m_Catalog.end()) {
                return false;
            }
            cat = it->second;
        }
        for (const auto& dep : cat.dependencies) {
            if (!LoadWithDependencies(dep)) {
                return false;
            }
        }
        return Acquire(guid) != nullptr;
    }

    std::shared_ptr<RuntimeAsset> LoadInternal(const we::runtime::assetimporter::AssetGuid& guid) {
        {
            std::lock_guard lock(m_Mutex);
            auto it = m_Resident.find(guid);
            if (it != m_Resident.end() && it->second
                && it->second->state == AssetLoadState::Loaded
                && !it->second->payload.empty()) {
                return it->second;
            }
        }

        CatalogEntry cat;
        {
            std::lock_guard lock(m_Mutex);
            const auto it = m_Catalog.find(guid);
            if (it == m_Catalog.end()) {
                return nullptr;
            }
            cat = it->second;
        }

        auto asset = std::make_shared<RuntimeAsset>();
        asset->guid = guid;
        asset->kind = cat.kind;
        asset->displayName = cat.displayName;
        asset->sourcePath = cat.loosePath;
        asset->contentType = cat.contentType;
        asset->state = AssetLoadState::Loading;

        if (cat.inPackage) {
            if (cat.packageOffset + cat.packageSize > m_Package.payloadBlob.size()) {
                asset->state = AssetLoadState::Failed;
                return nullptr;
            }
            asset->payload.assign(
                m_Package.payloadBlob.begin() + static_cast<std::ptrdiff_t>(cat.packageOffset),
                m_Package.payloadBlob.begin()
                    + static_cast<std::ptrdiff_t>(cat.packageOffset + cat.packageSize));
        } else {
            std::ifstream in(cat.loosePath, std::ios::binary);
            if (!in) {
                asset->state = AssetLoadState::Failed;
                return nullptr;
            }
            in.seekg(0, std::ios::end);
            const auto size = static_cast<size_t>(in.tellg());
            in.seekg(0, std::ios::beg);
            asset->payload.resize(size);
            if (size > 0) {
                in.read(reinterpret_cast<char*>(asset->payload.data()),
                    static_cast<std::streamsize>(size));
            }
            if (auto pkg = we::runtime::assetimporter::ReadAssetPackage(cat.loosePath)) {
                asset->kind = pkg->metadata.kind;
                asset->displayName = pkg->metadata.displayName;
                asset->contentType = pkg->payloadContentType;
                // Prefer decoded payload from package when present.
                if (!pkg->payload.empty()) {
                    asset->payload = std::move(pkg->payload);
                }
            }
        }

        asset->residentBytes = asset->payload.size();
        asset->state = AssetLoadState::Loaded;

        {
            std::lock_guard lock(m_Mutex);
            m_ResidentBytes += asset->residentBytes;
            m_Resident[guid] = asset;
            TouchLru(guid);
        }
        return asset;
    }

    void TouchLru(const we::runtime::assetimporter::AssetGuid& guid) {
        m_Lru.remove(guid);
        m_Lru.push_back(guid);
    }

    void EnsureWorkers() {
        std::lock_guard lock(m_Mutex);
        if (!m_Workers.empty()) {
            return;
        }
        unsigned count = m_Deps.workerThreads;
        if (count == 0) {
            count = std::min(2u, std::max(1u, std::thread::hardware_concurrency()));
        }
        m_Stopping = false;
        for (unsigned i = 0; i < count; ++i) {
            m_Workers.emplace_back([this] { WorkerLoop(); });
        }
    }

    void WorkerLoop() {
        for (;;) {
            AsyncLoadJob job;
            {
                std::unique_lock lock(m_Mutex);
                m_Cv.wait(lock, [&] { return m_Stopping || !m_LoadQueue.empty(); });
                if (m_Stopping && m_LoadQueue.empty()) {
                    return;
                }
                job = std::move(m_LoadQueue.front());
                m_LoadQueue.pop_front();
                ++m_ActiveWorkers;
            }

            if (job.onProgress) {
                AssetLoadProgress p{};
                p.guid = job.guid;
                p.state = AssetLoadState::Loading;
                p.fraction = 0.1f;
                job.onProgress(p);
            }

            auto asset = LoadInternal(job.guid);
            if (asset) {
                std::lock_guard lock(m_Mutex);
                ++asset->refCount;
                TouchLru(job.guid);
            }
            EnforceBudget();

            if (job.onProgress) {
                AssetLoadProgress p{};
                p.guid = job.guid;
                p.state = asset ? AssetLoadState::Loaded : AssetLoadState::Failed;
                p.fraction = 1.0f;
                job.onProgress(p);
            }
            if (job.onLoaded) {
                job.onLoaded(asset);
            }

            {
                std::lock_guard lock(m_Mutex);
                --m_ActiveWorkers;
            }
        }
    }

    AssetRuntimeDependencies m_Deps{};
    std::filesystem::path m_ContentRoot;
    std::filesystem::path m_PackagePath;
    we::runtime::assetcooker::AssetPackageArchive m_Package{};

    mutable std::mutex m_Mutex;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        CatalogEntry,
        we::runtime::assetimporter::AssetGuidHash>
        m_Catalog;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        std::shared_ptr<RuntimeAsset>,
        we::runtime::assetimporter::AssetGuidHash>
        m_Resident;
    std::list<we::runtime::assetimporter::AssetGuid> m_Lru;
    uint64_t m_ResidentBytes = 0;
    std::atomic<uint64_t> m_Budget{ 512ull * 1024ull * 1024ull };

    std::condition_variable m_Cv;
    std::deque<AsyncLoadJob> m_LoadQueue;
    std::vector<std::thread> m_Workers;
    unsigned m_ActiveWorkers = 0;
    bool m_Stopping = false;
};

} // namespace

std::unique_ptr<IAssetManager> CreateAssetManager(AssetRuntimeDependencies deps) {
    return std::make_unique<AssetManagerService>(std::move(deps));
}

} // namespace we::runtime::assetruntime
