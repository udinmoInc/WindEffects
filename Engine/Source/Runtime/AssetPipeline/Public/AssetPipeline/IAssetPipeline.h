#pragma once

#include "AssetImporter/IAssetImportService.h"
#include "AssetPipeline/BuildCache.h"
#include "AssetPipeline/DependencyGraph.h"
#include "AssetPipeline/Export.h"
#include "AssetPipeline/PipelineTypes.h"
#include "AssetPipeline/SourceWatcher.h"
#include "AssetProcessors/IAssetProcessorService.h"
#include "Platform/IPlatform.h"

#include <memory>
#include <optional>

namespace we::runtime::assetpipeline {

struct ASSETPIPELINE_API AssetPipelineDependencies {
    /// Required: existing importer entry point (never owned if external).
    we::runtime::assetimporter::IAssetImportService* importService = nullptr;
    /// Required: processor stage runner.
    we::runtime::assetprocessors::IAssetProcessorService* processorService = nullptr;
    /// Optional: for filesystem watching / hot reimport.
    we::platform::IPlatform* platform = nullptr;

    std::string engineVersion = "0.1.0";
    std::filesystem::path cacheRoot; // default: Intermediate/AssetPipeline
    unsigned workerThreads = 0;      // 0 = hardware_concurrency clamped
    std::function<void(const PipelineDiagnostic&)> onDiagnostic;
    std::function<void(const PipelineAssetResult&)> onAssetBuilt;
};

/// Orchestrates Import → Process → Cache → Dependency rebuild.
class ASSETPIPELINE_API IAssetPipeline {
public:
    virtual ~IAssetPipeline() = default;

    [[nodiscard]] virtual BuildCache& GetCache() = 0;
    [[nodiscard]] virtual const BuildCache& GetCache() const = 0;
    [[nodiscard]] virtual DependencyGraph& GetDependencyGraph() = 0;
    [[nodiscard]] virtual const DependencyGraph& GetDependencyGraph() const = 0;

    /// Single-asset build (import if needed, process, update cache/graph).
    [[nodiscard]] virtual PipelineResult BuildSync(const PipelineRequest& request) = 0;

    [[nodiscard]] virtual we::runtime::assetimporter::AssetGuid BuildAsync(
        const PipelineRequest& request,
        PipelineProgressCallback onProgress = {},
        PipelineCompletedCallback onCompleted = {}) = 0;

    /// Rebuild all assets under a content/source root (incremental).
    [[nodiscard]] virtual PipelineResult BuildDirectorySync(
        const std::filesystem::path& sourceRoot,
        const std::filesystem::path& outputDirectory,
        const PipelineRequest& defaults = {}) = 0;

    /// Invalidate cache and rebuild dependents of a changed source.
    [[nodiscard]] virtual PipelineResult RebuildDependentsSync(
        const we::runtime::assetimporter::AssetGuid& changedGuid) = 0;

    [[nodiscard]] virtual std::optional<PipelineProgress> GetJobProgress(
        const we::runtime::assetimporter::AssetGuid& jobId) const = 0;
    virtual bool CancelJob(const we::runtime::assetimporter::AssetGuid& jobId) = 0;
    virtual void WaitForIdle() = 0;
    virtual void Shutdown() = 0;

    /// Hot reimport: start watching a source directory (requires platform).
    [[nodiscard]] virtual bool StartWatching(const std::filesystem::path& sourceDirectory) = 0;
    virtual void StopWatching() = 0;
    /// Call from main loop after Platform::PollEvents when watching.
    virtual void PollWatchers() = 0;

    [[nodiscard]] virtual std::string GenerateValidationReport() const = 0;
};

[[nodiscard]] ASSETPIPELINE_API std::unique_ptr<IAssetPipeline> CreateAssetPipeline(
    AssetPipelineDependencies deps);

} // namespace we::runtime::assetpipeline
