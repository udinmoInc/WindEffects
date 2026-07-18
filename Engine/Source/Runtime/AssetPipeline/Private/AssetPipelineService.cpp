#include "AssetPipeline/IAssetPipeline.h"

#include "AssetImporter/AssetPackage.h"
#include "AssetImporter/IAssetImporter.h"
#include "AssetProcessors/ProcessTypes.h"
#include "Core/Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <sstream>
#include <thread>

namespace we::runtime::assetpipeline {
namespace {

uint64_t FileTimestamp(const std::filesystem::path& path) {
    std::error_code ec;
    const auto ft = std::filesystem::last_write_time(path, ec);
    if (ec) {
        return 0;
    }
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(ft.time_since_epoch()).count());
}

std::string HashText(std::string_view text) {
    uint64_t hash = 1469598103934665603ull;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    std::ostringstream oss;
    oss << std::hex << hash;
    return oss.str();
}

std::string ComputeSettingsHash(const we::runtime::assetimporter::ImportRequest& request) {
#if WE_HAS_NLOHMANN_JSON
    return HashText(request.settings.dump());
#else
    (void)request;
    return "0";
#endif
}

bool IsLikelySourceAsset(const std::filesystem::path& path) {
    const auto ext = path.extension().string();
    if (ext.empty()) {
        return false;
    }
    // Skip already-cooked / metadata.
    if (ext == ".weasset" || ext == ".wemeta" || ext == ".wepak" || ext == ".thumb"
        || ext == ".wefont" || ext == ".weiconatlas") {
        return false;
    }
    return true;
}

struct AsyncPipelineJob {
    we::runtime::assetimporter::AssetGuid id{};
    PipelineRequest request{};
    PipelineProgressCallback onProgress;
    PipelineCompletedCallback onCompleted;
    std::atomic<bool> cancel{ false };
    PipelineProgress progress{};
};

class AssetPipelineService final : public IAssetPipeline {
public:
    explicit AssetPipelineService(AssetPipelineDependencies deps)
        : m_Deps(std::move(deps))
        , m_Cache(m_Deps.cacheRoot.empty()
              ? std::filesystem::path("Intermediate") / "AssetPipeline"
              : m_Deps.cacheRoot)
        , m_Watcher(m_Deps.platform) {
        if (!m_Deps.importService || !m_Deps.processorService) {
            HE_ERROR("[AssetPipeline] importService and processorService are required.");
        }
        (void)m_Cache.Load();
        m_Watcher.SetCallback([this](const std::vector<WatchedChange>& changes) {
            OnSourcesChanged(changes);
        });
    }

    ~AssetPipelineService() override { Shutdown(); }

    [[nodiscard]] BuildCache& GetCache() override { return m_Cache; }
    [[nodiscard]] const BuildCache& GetCache() const override { return m_Cache; }
    [[nodiscard]] DependencyGraph& GetDependencyGraph() override { return m_Graph; }
    [[nodiscard]] const DependencyGraph& GetDependencyGraph() const override { return m_Graph; }

    [[nodiscard]] PipelineResult BuildSync(const PipelineRequest& request) override {
        return ExecuteBuild(request, {});
    }

    [[nodiscard]] we::runtime::assetimporter::AssetGuid BuildAsync(
        const PipelineRequest& request,
        PipelineProgressCallback onProgress,
        PipelineCompletedCallback onCompleted) override {
        EnsureWorkers();
        auto job = std::make_shared<AsyncPipelineJob>();
        job->id = we::runtime::assetimporter::AssetGuid::Generate();
        job->request = request;
        job->onProgress = std::move(onProgress);
        job->onCompleted = std::move(onCompleted);
        job->progress.jobId = job->id;
        job->progress.state = PipelineJobState::Queued;
        {
            std::lock_guard lock(m_Mutex);
            m_Jobs[job->id] = job;
            m_Queue.push_back(job);
        }
        m_Cv.notify_one();
        return job->id;
    }

    [[nodiscard]] PipelineResult BuildDirectorySync(
        const std::filesystem::path& sourceRoot,
        const std::filesystem::path& outputDirectory,
        const PipelineRequest& defaults) override {
        PipelineResult aggregate{};
        aggregate.success = true;
        if (!std::filesystem::exists(sourceRoot)) {
            PipelineDiagnostic d{};
            d.severity = we::runtime::assetimporter::ImportSeverity::Fatal;
            d.code = "pipeline.source_missing";
            d.message = "Source root does not exist.";
            d.assetPath = sourceRoot.string();
            aggregate.diagnostics.push_back(d);
            aggregate.success = false;
            return aggregate;
        }

        std::vector<std::filesystem::path> sources;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceRoot)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            if (IsLikelySourceAsset(entry.path())) {
                sources.push_back(entry.path());
            }
        }

        for (const auto& source : sources) {
            PipelineRequest req = defaults;
            req.import.sourcePath = source;
            req.import.outputDirectory = outputDirectory;
            req.import.assetName = source.stem().string();
            auto one = ExecuteBuild(req, {});
            for (auto& a : one.assets) {
                aggregate.assets.push_back(std::move(a));
            }
            for (auto& d : one.diagnostics) {
                aggregate.diagnostics.push_back(std::move(d));
            }
            aggregate.rebuiltCount += one.rebuiltCount;
            aggregate.skippedCount += one.skippedCount;
            aggregate.failedCount += one.failedCount;
            if (!one.success) {
                aggregate.success = false;
            }
        }
        (void)m_Cache.Save();
        return aggregate;
    }

    [[nodiscard]] PipelineResult RebuildDependentsSync(
        const we::runtime::assetimporter::AssetGuid& changedGuid) override {
        PipelineResult aggregate{};
        aggregate.success = true;
        const auto closure = m_Graph.CollectRebuildClosure(changedGuid);
        for (const auto& guid : closure) {
            auto entry = m_Cache.Find(guid);
            if (!entry) {
                continue;
            }
            PipelineRequest req{};
            req.forceRebuild = true;
            req.import.sourcePath = entry->sourcePath;
            req.import.outputDirectory = std::filesystem::path(entry->cookedPath).parent_path();
            req.import.existingGuid = guid;
            req.platformTarget = entry->platformTarget;
            auto one = ExecuteBuild(req, {});
            for (auto& a : one.assets) {
                aggregate.assets.push_back(std::move(a));
            }
            aggregate.rebuiltCount += one.rebuiltCount;
            aggregate.failedCount += one.failedCount;
            if (!one.success) {
                aggregate.success = false;
            }
        }
        (void)m_Cache.Save();
        return aggregate;
    }

    [[nodiscard]] std::optional<PipelineProgress> GetJobProgress(
        const we::runtime::assetimporter::AssetGuid& jobId) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Jobs.find(jobId);
        if (it == m_Jobs.end()) {
            return std::nullopt;
        }
        return it->second->progress;
    }

    bool CancelJob(const we::runtime::assetimporter::AssetGuid& jobId) override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Jobs.find(jobId);
        if (it == m_Jobs.end()) {
            return false;
        }
        it->second->cancel.store(true);
        it->second->progress.state = PipelineJobState::Cancelled;
        return true;
    }

    void WaitForIdle() override {
        for (;;) {
            {
                std::lock_guard lock(m_Mutex);
                if (m_Queue.empty() && m_ActiveWorkers == 0) {
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
        StopWatching();
        (void)m_Cache.Save();
    }

    [[nodiscard]] bool StartWatching(const std::filesystem::path& sourceDirectory) override {
        m_WatchRoot = sourceDirectory;
        return m_Watcher.Start(sourceDirectory, true);
    }

    void StopWatching() override { m_Watcher.Stop(); }

    void PollWatchers() override {
        if (m_Deps.platform) {
            m_Watcher.Poll(*m_Deps.platform);
        }
    }

    [[nodiscard]] std::string GenerateValidationReport() const override {
        std::ostringstream oss;
        oss << "Asset Pipeline Validation Report\n";
        oss << "Cache entries: " << m_Cache.Size() << "\n";
        oss << "Graph assets: " << m_Graph.AssetCount() << "\n";
        oss << "Cycles detected: " << (m_Graph.HasCycles() ? "yes" : "no") << "\n";
        return oss.str();
    }

private:
    void EmitDiag(PipelineResult& result, PipelineDiagnostic d) {
        if (m_Deps.onDiagnostic) {
            m_Deps.onDiagnostic(d);
        }
        result.diagnostics.push_back(std::move(d));
    }

    PipelineResult ExecuteBuild(
        const PipelineRequest& request,
        const PipelineProgressCallback& progressCb) {
        PipelineResult result{};
        result.success = false;

        if (!m_Deps.importService || !m_Deps.processorService) {
            EmitDiag(result, {
                we::runtime::assetimporter::ImportSeverity::Fatal,
                "pipeline.missing_deps",
                "AssetPipeline requires import and processor services.",
                request.import.sourcePath.string()
            });
            return result;
        }

        auto reportProgress = [&](PipelineJobState state, float fraction, std::string stage) {
            if (!progressCb) {
                return;
            }
            PipelineProgress p{};
            p.state = state;
            p.fraction = fraction;
            p.stage = std::move(stage);
            p.totalAssets = 1;
            p.completedAssets = (state == PipelineJobState::Succeeded || state == PipelineJobState::Skipped) ? 1u : 0u;
            progressCb(p);
        };

        const auto& sourcePath = request.import.sourcePath;
        if (!std::filesystem::exists(sourcePath)) {
            EmitDiag(result, {
                we::runtime::assetimporter::ImportSeverity::Fatal,
                "pipeline.source_missing",
                "Source file does not exist.",
                sourcePath.string()
            });
            return result;
        }

        const std::string sourceHash = we::runtime::assetimporter::ComputeFileHashHex(sourcePath);
        const uint64_t sourceTs = FileTimestamp(sourcePath);
        const std::string settingsHash = ComputeSettingsHash(request.import);
        const std::string processorFp = m_Deps.processorService->GetRegistry().ComputeFingerprint();

        // Probe importer id/version via detect + registry when possible.
        std::string importerId = "unknown";
        std::string importerVersion = "1.0.0";
        const auto kind = request.import.forcedKind != we::runtime::assetimporter::AssetKind::Unknown
            ? request.import.forcedKind
            : m_Deps.importService->DetectKind(sourcePath);
        for (const auto& importer : m_Deps.importService->GetRegistry().GetAll()) {
            if (importer && importer->GetAssetKind() == kind) {
                importerId = std::string(importer->GetImporterId());
                break;
            }
        }

        const std::string cacheKey = BuildCache::MakeCacheKey(
            sourceHash,
            sourceTs,
            importerId,
            importerVersion,
            processorFp,
            settingsHash,
            m_Deps.engineVersion,
            request.platformTarget);

        if (!request.forceRebuild) {
            if (auto hit = m_Cache.FindByCacheKey(cacheKey)) {
                if (!hit->cookedPath.empty() && std::filesystem::exists(hit->cookedPath)) {
                    PipelineAssetResult skipped{};
                    skipped.success = true;
                    skipped.skipped = true;
                    skipped.cacheKey = cacheKey;
                    skipped.asset.metadata.guid = hit->guid;
                    skipped.asset.cookedPath = hit->cookedPath;
                    skipped.asset.metadata.sourceHash = hit->sourceHash;
                    skipped.asset.metadata.sourcePath = hit->sourcePath;
                    result.assets.push_back(std::move(skipped));
                    result.skippedCount = 1;
                    result.success = true;
                    reportProgress(PipelineJobState::Skipped, 1.0f, "CacheHit");
                    return result;
                }
            }
        }

        reportProgress(PipelineJobState::Importing, 0.1f, "Import");
        auto importResult = m_Deps.importService->ImportSync(request.import);
        for (const auto& d : importResult.diagnostics) {
            EmitDiag(result, { d.severity, d.code, d.message, d.contextPath });
        }
        if (!importResult.success) {
            result.failedCount = 1;
            return result;
        }

        reportProgress(PipelineJobState::Processing, 0.5f, "Process");
        we::runtime::assetprocessors::ProcessContext processCtx{};
        processCtx.imported = importResult.asset;
        processCtx.intermediateRoot = request.intermediateRoot.empty()
            ? m_Cache.GetRoot() / "Intermediate"
            : request.intermediateRoot;
        processCtx.processedRoot = request.processedRoot.empty()
            ? m_Cache.GetRoot() / "Processed"
            : request.processedRoot;
        processCtx.engineVersion = m_Deps.engineVersion;
        processCtx.platformTarget = request.platformTarget;
        processCtx.generateThumbnail = request.generateThumbnail;
        processCtx.enableOptimization = request.enableOptimization;

        auto processResult = m_Deps.processorService->Process(std::move(processCtx));
        for (const auto& d : processResult.diagnostics) {
            EmitDiag(result, { d.severity, d.code, d.message, processResult.asset.cookedPath.string() });
        }
        if (!processResult.success) {
            result.failedCount = 1;
            return result;
        }

        reportProgress(PipelineJobState::Caching, 0.9f, "Cache");
        m_Graph.SetDependencies(processResult.asset.metadata.guid, processResult.asset.metadata.dependencies);

        BuildCacheEntry entry{};
        entry.cacheKey = cacheKey;
        entry.guid = processResult.asset.metadata.guid;
        entry.sourcePath = sourcePath.string();
        entry.sourceHash = sourceHash;
        entry.sourceTimestamp = sourceTs;
        entry.importerId = processResult.asset.metadata.importerId.empty()
            ? importerId
            : processResult.asset.metadata.importerId;
        entry.importerVersion = processResult.asset.metadata.importerVersion.empty()
            ? importerVersion
            : processResult.asset.metadata.importerVersion;
        entry.processorFingerprint = processResult.processorFingerprint.empty()
            ? processorFp
            : processResult.processorFingerprint;
        entry.settingsHash = settingsHash;
        entry.cookedPath = processResult.asset.cookedPath.string();
        entry.cookedHash = we::runtime::assetimporter::ComputeFileHashHex(processResult.asset.cookedPath);
        entry.engineVersion = m_Deps.engineVersion;
        entry.platformTarget = request.platformTarget;
        m_Cache.Upsert(std::move(entry));
        (void)m_Cache.Save();

        PipelineAssetResult built{};
        built.success = true;
        built.skipped = false;
        built.asset = processResult.asset;
        built.process = std::move(processResult);
        built.cacheKey = cacheKey;
        if (m_Deps.onAssetBuilt) {
            m_Deps.onAssetBuilt(built);
        }
        result.assets.push_back(std::move(built));
        result.rebuiltCount = 1;
        result.success = true;
        reportProgress(PipelineJobState::Succeeded, 1.0f, "Done");
        return result;
    }

    void OnSourcesChanged(const std::vector<WatchedChange>& changes) {
        for (const auto& change : changes) {
            if (change.action == we::platform::DirectoryWatchAction::Removed) {
                continue;
            }
            std::filesystem::path path(change.path);
            if (!IsLikelySourceAsset(path)) {
                continue;
            }
            PipelineRequest req{};
            req.import.sourcePath = path;
            req.import.outputDirectory = m_WatchOutput.empty()
                ? (m_WatchRoot / "Cooked")
                : m_WatchOutput;
            req.forceRebuild = true;
            auto result = ExecuteBuild(req, {});
            if (result.success && !result.assets.empty()) {
                const auto& guid = result.assets.front().asset.metadata.guid;
                if (!guid.IsNil()) {
                    (void)RebuildDependentsSync(guid);
                }
            }
        }
    }

    void EnsureWorkers() {
        std::lock_guard lock(m_Mutex);
        if (!m_Workers.empty()) {
            return;
        }
        unsigned count = m_Deps.workerThreads;
        if (count == 0) {
            count = std::thread::hardware_concurrency();
            if (count == 0) {
                count = 2;
            }
            count = std::min(count, 4u);
        }
        m_Stopping = false;
        for (unsigned i = 0; i < count; ++i) {
            m_Workers.emplace_back([this] { WorkerLoop(); });
        }
    }

    void WorkerLoop() {
        for (;;) {
            std::shared_ptr<AsyncPipelineJob> job;
            {
                std::unique_lock lock(m_Mutex);
                m_Cv.wait(lock, [&] { return m_Stopping || !m_Queue.empty(); });
                if (m_Stopping && m_Queue.empty()) {
                    return;
                }
                job = m_Queue.front();
                m_Queue.pop_front();
                ++m_ActiveWorkers;
            }

            if (job->cancel.load()) {
                if (job->onCompleted) {
                    PipelineResult cancelled{};
                    cancelled.jobId = job->id;
                    cancelled.success = false;
                    job->onCompleted(cancelled);
                }
            } else {
                job->progress.state = PipelineJobState::Importing;
                auto result = ExecuteBuild(job->request, job->onProgress);
                result.jobId = job->id;
                if (job->onCompleted) {
                    job->onCompleted(result);
                }
                job->progress.state = result.success ? PipelineJobState::Succeeded : PipelineJobState::Failed;
                job->progress.fraction = 1.0f;
            }

            {
                std::lock_guard lock(m_Mutex);
                --m_ActiveWorkers;
            }
        }
    }

    AssetPipelineDependencies m_Deps{};
    BuildCache m_Cache;
    DependencyGraph m_Graph{};
    SourceWatcher m_Watcher;
    std::filesystem::path m_WatchRoot;
    std::filesystem::path m_WatchOutput;

    mutable std::mutex m_Mutex;
    std::condition_variable m_Cv;
    std::deque<std::shared_ptr<AsyncPipelineJob>> m_Queue;
    std::unordered_map<
        we::runtime::assetimporter::AssetGuid,
        std::shared_ptr<AsyncPipelineJob>,
        we::runtime::assetimporter::AssetGuidHash>
        m_Jobs;
    std::vector<std::thread> m_Workers;
    unsigned m_ActiveWorkers = 0;
    bool m_Stopping = false;
};

} // namespace

std::unique_ptr<IAssetPipeline> CreateAssetPipeline(AssetPipelineDependencies deps) {
    return std::make_unique<AssetPipelineService>(std::move(deps));
}

} // namespace we::runtime::assetpipeline
