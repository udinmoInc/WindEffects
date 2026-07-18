#pragma once

#include "AssetImporter/AssetGuid.h"
#include "AssetImporter/ImportTypes.h"
#include "AssetPipeline/Export.h"
#include "AssetProcessors/ProcessTypes.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace we::runtime::assetpipeline {

enum class PipelineJobState : uint32_t {
    Queued = 0,
    Importing,
    Processing,
    Caching,
    Succeeded,
    Failed,
    Cancelled,
    Skipped
};

struct ASSETPIPELINE_API PipelineRequest {
    we::runtime::assetimporter::ImportRequest import{};
    std::filesystem::path intermediateRoot; // Intermediate/AssetPipeline
    std::filesystem::path processedRoot;    // optional processed output mirror
    std::string platformTarget = "Windows";
    bool forceRebuild = false;
    bool generateThumbnail = true;
    bool enableOptimization = true;
    bool watchSources = false;
};

struct ASSETPIPELINE_API PipelineDiagnostic {
    we::runtime::assetimporter::ImportSeverity severity =
        we::runtime::assetimporter::ImportSeverity::Info;
    std::string code;
    std::string message;
    std::string assetPath;
};

struct ASSETPIPELINE_API PipelineProgress {
    we::runtime::assetimporter::AssetGuid jobId{};
    PipelineJobState state = PipelineJobState::Queued;
    float fraction = 0.0f;
    std::string stage;
    std::string detail;
    uint32_t completedAssets = 0;
    uint32_t totalAssets = 0;
};

struct ASSETPIPELINE_API PipelineAssetResult {
    bool success = false;
    bool skipped = false; // cache hit
    we::runtime::assetimporter::ImportedAsset asset{};
    we::runtime::assetprocessors::ProcessResult process{};
    std::string cacheKey;
};

struct ASSETPIPELINE_API PipelineResult {
    bool success = false;
    we::runtime::assetimporter::AssetGuid jobId{};
    std::vector<PipelineAssetResult> assets;
    std::vector<PipelineDiagnostic> diagnostics;
    uint32_t rebuiltCount = 0;
    uint32_t skippedCount = 0;
    uint32_t failedCount = 0;

    [[nodiscard]] std::string PrimaryErrorMessage() const;
};

using PipelineProgressCallback = std::function<void(const PipelineProgress&)>;
using PipelineCompletedCallback = std::function<void(const PipelineResult&)>;

} // namespace we::runtime::assetpipeline
