#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/Export.h"
#include "AssetImporter/ImportTypes.h"
#include "AssetImporter/ImporterRegistry.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace we::runtime::assetimporter {

/// Optional hooks injected by Editor / tools (Asset Registry, logging sinks, etc.).
struct ASSETIMPORTER_API AssetImportServiceDependencies {
    std::string engineVersion = "0.1.0";
    std::filesystem::path intermediateRoot; // optional Intermediate/Import cache
    std::function<void(const ImportedAsset&)> onAssetImported;
    std::function<void(const ImportDiagnostic&)> onDiagnostic;
    unsigned workerThreads = 0; // 0 = hardware_concurrency clamped
};

/// Unified import facade used by Editor, Content Browser, CLI, cook, and plugins.
class ASSETIMPORTER_API IAssetImportService {
public:
    virtual ~IAssetImportService() = default;

    [[nodiscard]] virtual ImporterRegistry& GetRegistry() = 0;
    [[nodiscard]] virtual const ImporterRegistry& GetRegistry() const = 0;

    /// Blocking import on the calling thread.
    [[nodiscard]] virtual ImportResult ImportSync(const ImportRequest& request) = 0;

    /// Queue for background workers. Returns job id immediately.
    [[nodiscard]] virtual AssetGuid ImportAsync(
        const ImportRequest& request,
        ImportProgressCallback onProgress = {},
        ImportCompletedCallback onCompleted = {}) = 0;

    [[nodiscard]] virtual std::optional<ImportProgress> GetJobProgress(const AssetGuid& jobId) const = 0;
    virtual bool CancelJob(const AssetGuid& jobId) = 0;
    virtual void WaitForIdle() = 0;
    virtual void Shutdown() = 0;

    [[nodiscard]] virtual AssetKind DetectKind(const std::filesystem::path& sourcePath) const = 0;
};

[[nodiscard]] ASSETIMPORTER_API std::unique_ptr<IAssetImportService> CreateAssetImportService(
    AssetImportServiceDependencies deps = {});

} // namespace we::runtime::assetimporter
