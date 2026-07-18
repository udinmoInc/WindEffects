#pragma once

#include "AssetCooker/CookTypes.h"
#include "AssetImporter/IAssetImportService.h"
#include "AssetPipeline/IAssetPipeline.h"
#include "ContentBrowser/Export.h"

#include <filesystem>
#include <memory>
#include <string>

namespace we::programs::editor {

/// Lazy shared import service for Content Browser / Editor UI.
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetimporter::IAssetImportService& GetEditorAssetImportService();

/// Lazy shared asset pipeline (import → process → cache) for Editor tooling.
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetpipeline::IAssetPipeline& GetEditorAssetPipeline();

/// Import a source file into the project content root (sync).
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetimporter::ImportResult ImportAssetToContent(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& contentOutputDirectory,
    const std::string& assetName = {});

/// Full pipeline build (import + processors + incremental cache).
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetpipeline::PipelineResult BuildAssetToContent(
    const std::filesystem::path& sourcePath,
    const std::filesystem::path& contentOutputDirectory,
    const std::string& assetName = {});

/// Cook project Content/ for a platform; optionally write a .wepak package.
[[nodiscard]] CONTENTBROWSER_API we::runtime::assetcooker::CookResult CookProjectContent(
    const std::filesystem::path& contentRoot,
    const std::filesystem::path& outputDirectory,
    const std::filesystem::path& packagePath = {},
    const std::string& platformName = "Windows");

/// Lightweight results for Editor / menu callers (avoids linking asset DLLs directly).
struct CONTENTBROWSER_API AssetBridgeOpResult {
    bool success = false;
    std::string message;
    uint32_t rebuiltCount = 0;
    uint32_t skippedCount = 0;
    uint32_t failedCount = 0;
    uint32_t assetCount = 0;
    std::filesystem::path outputPath;
};

[[nodiscard]] CONTENTBROWSER_API AssetBridgeOpResult BuildContentDirectory(
    const std::filesystem::path& sourceRoot,
    const std::filesystem::path& outputDirectory);

[[nodiscard]] CONTENTBROWSER_API AssetBridgeOpResult CookOrPackageContent(
    const std::filesystem::path& contentRoot,
    const std::filesystem::path& outputDirectory,
    const std::filesystem::path& packagePath = {},
    const std::string& platformName = "Windows");

} // namespace we::programs::editor
