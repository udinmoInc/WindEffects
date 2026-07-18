#include "AssetImporter/IAssetImportService.h"

#include "Importers/BuiltinImporters.h"
#include "ImportHelpers.h"

#include "Core/Logger.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <thread>
#include <unordered_map>

namespace we::runtime::assetimporter {
namespace {

struct AsyncJob {
    AssetGuid id{};
    ImportRequest request{};
    ImportProgressCallback onProgress;
    ImportCompletedCallback onCompleted;
    std::atomic<bool> cancel{ false };
    ImportProgress progress{};
};

class AssetImportService final : public IAssetImportService {
public:
    explicit AssetImportService(AssetImportServiceDependencies deps)
        : m_Deps(std::move(deps)) {
        RegisterBuiltinImporters(m_Registry);
        // Workers start lazily on first ImportAsync to keep sync/CLI paths light.
    }

    ~AssetImportService() override {
        Shutdown();
    }

    [[nodiscard]] ImporterRegistry& GetRegistry() override { return m_Registry; }
    [[nodiscard]] const ImporterRegistry& GetRegistry() const override { return m_Registry; }

    [[nodiscard]] ImportResult ImportSync(const ImportRequest& request) override {
        return ExecuteImport(request, {});
    }

    [[nodiscard]] AssetGuid ImportAsync(
        const ImportRequest& request,
        ImportProgressCallback onProgress,
        ImportCompletedCallback onCompleted) override {
        EnsureWorkersStarted();

        auto job = std::make_shared<AsyncJob>();
        job->id = AssetGuid::Generate();
        job->request = request;
        job->onProgress = std::move(onProgress);
        job->onCompleted = std::move(onCompleted);
        job->progress.jobId = job->id;
        job->progress.state = ImportJobState::Queued;

        {
            std::lock_guard lock(m_Mutex);
            m_Jobs[job->id] = job;
            m_Queue.push_back(job);
        }
        m_Cv.notify_one();
        return job->id;
    }

    [[nodiscard]] std::optional<ImportProgress> GetJobProgress(const AssetGuid& jobId) const override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Jobs.find(jobId);
        if (it == m_Jobs.end()) {
            return std::nullopt;
        }
        return it->second->progress;
    }

    bool CancelJob(const AssetGuid& jobId) override {
        std::lock_guard lock(m_Mutex);
        const auto it = m_Jobs.find(jobId);
        if (it == m_Jobs.end()) {
            return false;
        }
        it->second->cancel.store(true);
        it->second->progress.state = ImportJobState::Cancelled;
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
        for (auto& worker : m_Workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
        m_Workers.clear();
    }

    [[nodiscard]] AssetKind DetectKind(const std::filesystem::path& sourcePath) const override {
        return AssetKindFromSourceExtension(sourcePath.extension().string());
    }

private:
    ImportResult ExecuteImport(const ImportRequest& request, const ImportProgressCallback& progress) {
        ImportResult result{};
        result.jobId = request.existingGuid.IsNil() ? AssetGuid::Generate() : request.existingGuid;

        if (!std::filesystem::exists(request.sourcePath)) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "source.missing",
                "Source path does not exist.", request.sourcePath.string());
            EmitDiagnostic(result);
            return result;
        }

        std::shared_ptr<IAssetImporter> importer;
        if (request.forcedKind != AssetKind::Unknown) {
            importer = m_Registry.ResolveForKind(request.forcedKind);
        }
        if (!importer) {
            importer = m_Registry.ResolveForPath(request.sourcePath);
        }
        if (!importer) {
            detail::AddDiagnostic(result, ImportSeverity::Fatal, "importer.not_found",
                "No importer registered for this asset.", request.sourcePath.string());
            EmitDiagnostic(result);
            return result;
        }

        ImportRequest local = request;
        if (local.outputDirectory.empty() && !m_Deps.intermediateRoot.empty()) {
            local.outputDirectory = m_Deps.intermediateRoot / "Imported";
        }

        result = importer->Import(local, progress);
        if (result.jobId.IsNil()) {
            result.jobId = local.existingGuid.IsNil() ? AssetGuid::Generate() : local.existingGuid;
        }
        if (result.asset.metadata.engineVersion.empty()) {
            result.asset.metadata.engineVersion = m_Deps.engineVersion;
        }

        EmitDiagnostic(result);
        if (result.success && m_Deps.onAssetImported) {
            m_Deps.onAssetImported(result.asset);
        }
        return result;
    }

    void EmitDiagnostic(const ImportResult& result) {
        if (!m_Deps.onDiagnostic) {
            return;
        }
        for (const auto& d : result.diagnostics) {
            m_Deps.onDiagnostic(d);
        }
    }

    void EnsureWorkersStarted() {
        std::lock_guard lock(m_Mutex);
        if (!m_Workers.empty() || m_Stopping) {
            return;
        }
        unsigned count = m_Deps.workerThreads;
        if (count == 0) {
            count = std::max(1u, std::thread::hardware_concurrency());
            count = std::min(count, 4u);
        }
        m_Workers.reserve(count);
        for (unsigned i = 0; i < count; ++i) {
            m_Workers.emplace_back([this]() { WorkerLoop(); });
        }
    }

    void WorkerLoop() {
        for (;;) {
            std::shared_ptr<AsyncJob> job;
            {
                std::unique_lock lock(m_Mutex);
                m_Cv.wait(lock, [this]() { return m_Stopping || !m_Queue.empty(); });
                if (m_Stopping && m_Queue.empty()) {
                    return;
                }
                job = m_Queue.front();
                m_Queue.pop_front();
                ++m_ActiveWorkers;
            }

            if (job->cancel.load()) {
                ImportResult cancelled{};
                cancelled.jobId = job->id;
                cancelled.success = false;
                detail::AddDiagnostic(cancelled, ImportSeverity::Warning, "job.cancelled", "Import cancelled.");
                job->progress.state = ImportJobState::Cancelled;
                if (job->onCompleted) {
                    job->onCompleted(cancelled);
                }
                std::lock_guard lock(m_Mutex);
                --m_ActiveWorkers;
                continue;
            }

            job->progress.state = ImportJobState::Running;
            auto progressCb = [job](const ImportProgress& p) {
                job->progress = p;
                job->progress.jobId = job->id;
                if (job->onProgress) {
                    job->onProgress(job->progress);
                }
            };

            ImportResult result = ExecuteImport(job->request, progressCb);
            result.jobId = job->id;
            job->progress.fraction = 1.0f;
            job->progress.state = result.success ? ImportJobState::Succeeded : ImportJobState::Failed;
            job->progress.stage = result.success ? "Completed" : "Failed";
            if (job->onCompleted) {
                job->onCompleted(result);
            }

            {
                std::lock_guard lock(m_Mutex);
                --m_ActiveWorkers;
                // Keep job progress queryable briefly; erase completed jobs.
                m_Jobs.erase(job->id);
            }
        }
    }

    AssetImportServiceDependencies m_Deps{};
    ImporterRegistry m_Registry{};

    mutable std::mutex m_Mutex;
    std::condition_variable m_Cv;
    std::deque<std::shared_ptr<AsyncJob>> m_Queue;
    std::unordered_map<AssetGuid, std::shared_ptr<AsyncJob>, AssetGuidHash> m_Jobs;
    std::vector<std::thread> m_Workers;
    unsigned m_ActiveWorkers = 0;
    bool m_Stopping = false;
};

} // namespace

std::unique_ptr<IAssetImportService> CreateAssetImportService(AssetImportServiceDependencies deps) {
    return std::make_unique<AssetImportService>(std::move(deps));
}

} // namespace we::runtime::assetimporter
